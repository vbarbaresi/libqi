/*
**  Copyright (C) 2012 Aldebaran Robotics
**  See COPYING for the license
*/
#include "sessionservice.hpp"
#include "servicedirectoryclient.hpp"
#include "sessionserver.hpp"
#include "serverclient.hpp"
#include "signal_p.hpp"
#include "src/remoteobject_p.hpp"

namespace qi {

  Session_Service::~Session_Service()
  {
    close();
  }

  void Session_Service::close() {
    //cleanup all RemoteObject
    //they are not valid anymore after this function
    {
      boost::mutex::scoped_lock sl(_remoteObjectsMutex);
      RemoteObjectMap::iterator it = _remoteObjects.begin();
      for (; it != _remoteObjects.end(); ++it) {
        reinterpret_cast<RemoteObject*>(it->second.value)->close();
      }
      _remoteObjects.clear();
    }
  }


  ServiceRequest *Session_Service::serviceRequest(long requestId)
  {
    {
      boost::mutex::scoped_lock                 sl(_requestsMutex);
      std::map<long, ServiceRequest*>::iterator it;

      it = _requests.find(requestId);
      if (it == _requests.end()) {
        qiLogVerbose("qi.session_service") << "qi.session.service(): No matching request for id(" << requestId << ").";
        return 0;
      }
      return it->second;
    }
  }

  void Session_Service::removeRequest(long requestId)
  {
    {
      boost::mutex::scoped_lock                 l(_requestsMutex);
      std::map<long, ServiceRequest*>::iterator it;

      it = _requests.find(requestId);
      if (it == _requests.end()) {
        qiLogVerbose("qi.session_service") << "qi.session.service(): No matching request for id(" << requestId << ").";
        return;
      }
      if (it->second) {
        ServerClient *sc = it->second->sclient;
        // LEAK, but socket ownership will be refactored and the pb will go away
        //delete sc;
        it->second->sclient = 0;
      }
      delete it->second;
      it->second = 0;
      _requests.erase(it);
    }
  }

  void Session_Service::onTransportSocketResult(qi::Future<TransportSocketPtr> value, long requestId) {
    ServiceRequest *sr = serviceRequest(requestId);
    if (!sr)
      return;

    if (value.hasError()) {
      sr->promise.setError(value.error());
      removeRequest(requestId);
      return;
    }

    sr->sclient = new qi::ServerClient(value.value());
    sr->socket = value.value();

    qi::Future<qi::MetaObject> fut = sr->sclient->metaObject(sr->serviceId, qi::Message::GenericObject_Main);
    fut.connect(boost::bind<void>(&Session_Service::onMetaObjectResult, this, _1, requestId));
  }

  void Session_Service::onMetaObjectResult(qi::Future<qi::MetaObject> mo, long requestId) {
    ServiceRequest *sr = serviceRequest(requestId);
    if (!sr)
      return;

    if (mo.hasError()) {
      sr->promise.setError(mo.error());
      removeRequest(requestId);
      return;
    }

    {
      boost::mutex::scoped_lock sl(_remoteObjectsMutex);
      RemoteObjectMap::iterator it = _remoteObjects.find(sr->name);
      if (it != _remoteObjects.end()) {
        //another object have been registered before us, return it
        //the new socket will be closed when the request is deleted
        qiLogVerbose("session_service") << "A request for the service " << sr->name << " have been discarded, "
                                        << "the remoteobject on the service was already available.";
        sr->promise.setValue(it->second);
      } else {
        RemoteObject* robj = new RemoteObject(sr->serviceId, mo, sr->socket);
        GenericObject o = makeDynamicObject(robj);
        // The remoteobject in sr->client.remoteObject is still on
        //remove the callback of ServerClient before returning the object
        //TODO: belong to TransportSocketCache
        sr->socket->connected._p->reset();
        sr->socket->disconnected._p->reset();
        sr->socket->messageReady._p->reset();


        //avoid deleting the socket in removeRequest (RemoteObject will do it)
        sr->socket.reset();
        //register the remote object in the cache
        _remoteObjects[sr->name] = o;
        sr->promise.setValue(o);
      }
    }

    removeRequest(requestId);
  }

  // We received a ServiceInfo, and want to establish a connection
  void Session_Service::onServiceInfoResult(qi::Future<qi::ServiceInfo> result, long requestId) {
    ServiceRequest *sr = serviceRequest(requestId);
    if (!sr)
      return;
    if (result.hasError()) {
      sr->promise.setError(result.error());
      removeRequest(requestId);
      return;
    }
    qi::ServiceInfo &si = result.value();
    sr->serviceId = si.serviceId();
    //empty serviceInfo
    if (!si.endpoints().size()) {
      std::stringstream ss;
      ss << "No endpoints returned for service:" << sr->name << " (id:" << sr->serviceId << ")";
      qiLogVerbose("session.service") << ss.str();
      sr->promise.setError(ss.str());
      removeRequest(requestId);
      return;
    }

    qi::Future<qi::TransportSocketPtr> fut = _socketCache->socket(si.endpoints());
    fut.connect(boost::bind<void>(&Session_Service::onTransportSocketResult, this, _1, requestId));
  }

  qi::Future<qi::GenericObject> Session_Service::service(const std::string &service,
                                                         Session::ServiceLocality locality,
                                                         const std::string &type)
  {
    qiLogVerbose("session.service") << "Getting service " << service;
    qi::Future<qi::GenericObject> result;
    if (locality != Session::ServiceLocality_Remote) {
      //qiLogError("session.service") << "service is not implemented for local service, it always return a remote service";
      //look for local object registered in the server
      qi::GenericObject go = _server->registeredServiceObject(service);
      if (go.isValid())
        return qi::Future<qi::GenericObject>(go);
      if (locality == Session::ServiceLocality_Local) {
        qi::Promise<qi::GenericObject> prom;
        prom.setError(std::string("No local object found for ") + service);
        return prom.future();
      }
    }

    //look for already registered remote objects
    {
      boost::mutex::scoped_lock sl(_remoteObjectsMutex);
      RemoteObjectMap::iterator it = _remoteObjects.find(service);
      if (it != _remoteObjects.end()) {
        return qi::Future<qi::GenericObject>(it->second);
      }
    }

    qi::Future<qi::ServiceInfo> fut = _sdClient->service(service);
    ServiceRequest *rq = new ServiceRequest(service, type);
    long requestId = ++_requestsIndex;

    {
      boost::mutex::scoped_lock l(_requestsMutex);
      _requests[requestId] = rq;
    }
    result = rq->promise.future();
    //rq is not valid anymore after addCallbacks, because it could have been handled and cleaned
    fut.connect(boost::bind<void>(&Session_Service::onServiceInfoResult, this, _1, requestId));
    return result;
  }


}

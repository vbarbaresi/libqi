/*
** Author(s):
**  - Laurent Lec <llec@aldebaran-robotics.com>
**
** Copyright (C) 2012 Aldebaran Robotics
*/

#include <iostream>
#include <string>
#include <ctime>

#include <qi/application.hpp>
#include <qi/log.hpp>
#include <qimessaging/session.hpp>
#include <qimessaging/gateway.hpp>

#include <sqlite3.h>

class RemoteConnection: public qi::TransportServerInterface,
                        public qi::TransportSocketInterface
{
public:
  enum State
  {
    State_Waiting      = 1,
    State_Connected    = 2,
    State_Disconnected = 3,
  };

  RemoteConnection(qi::TransportSocket *socket)
    : _state(State_Waiting)
    , _creationTime(time(0))
    , _socket(0)
    , _updated(false)
    , _originalSocket(socket)
  {
  }

  bool updated()
  {
    bool r = _updated;
    _updated = false;
    return r;
  }

  qi::RemoteGateway *remoteGateway()
  {
    return &_remoteGateway;
  }

  void newConnection(qi::TransportServer* server, qi::TransportSocket *socket)
  {
    _state = State_Connected;
    _socket = socket;
    _connectionTime = time(0);
    _updated = true;
    _originalSocket->disconnect();
  }

  void onSocketConnected(qi::TransportSocket *, void *data) { }

  void onSocketConnectionError(qi::TransportSocket *, void *data) { }

  void onSocketDisconnected(qi::TransportSocket *socket, void *data)
  {
    _state = State_Disconnected;
    _updated = true;
  }

  void onSocketReadyRead(qi::TransportSocket *socket,
                         int QI_UNUSED(id), void *data)
  {
    _lastPingTime = time(0);
    _updated = true;
  }

private:
  State                _state;
  const time_t         _creationTime;
  time_t               _connectionTime;
  time_t               _lastPingTime;
  qi::RemoteGateway    _remoteGateway;
  qi::TransportSocket *_socket;
  bool                 _updated;
  qi::TransportSocket *_originalSocket;
};

class LoadBalancer: public qi::TransportServerInterface,
                    public qi::TransportSocketInterface
{
public:
  LoadBalancer(const qi::Url &listenURL)
    : _listenURL(listenURL)
    , _server(_listenURL)
  {
    qiLogInfo("lb") << "Will listen on " << _listenURL.str();


    sqlite3_open("/tmp/db.sql", &_db);
    const char* req = "CREATE TABLE robots ("
                        "name            text,"
                        "endpoint        text,"
                        "connection_time integer,"
                        "lastping_time   integer"
                      ")";
    char *err;
    sqlite3_exec(_db, req, 0, 0, &err);
  }

  bool listen()
  {
    _server.addCallbacks(this);
    return _server.listen();
  }

  void newConnection(qi::TransportServer* server, qi::TransportSocket *socket)
  {
    socket->addCallbacks(this);
  }

  void onSocketDisconnected(qi::TransportSocket *socket, void *data)
  {
  }

  void onSocketReadyRead(qi::TransportSocket *socket, int id, void *data)
  {
    qi::Message msg;
    socket->read(id, &msg);

    if (msg.service() == qi::Message::Service_Server &&
        msg.function() == qi::Message::ServerFunction_Connect)
    {
      // forge reply
      qi::Buffer buf;
      qi::Message ans;
      ans.setBuffer(buf);
      ans.setService(qi::Message::Service_Server);
      ans.setType(qi::Message::Type_Reply);
      ans.setFunction(qi::Message::ServerFunction_Connect);
      ans.setObject(qi::Message::Object_Main);
      qi::ODataStream d(buf);

      RemoteConnection *rc = new RemoteConnection(socket);

      qi::Url url;
      do
      {
        std::stringstream ss;
        ss << "tcp://" << _listenURL.host() << ':' << qi::os::findAvailablePort(22002);
        url = ss.str();
      }
      while (!rc->remoteGateway()->listen(url));

      _clients[rc->remoteGateway()] = rc;

      rc->remoteGateway()->addCallbacks(rc, rc);

      qiLogInfo("lb") << "Spawned RemoteGateway on " << url.str();

      d << url.str();
      socket->send(ans);

      sqlite3_stmt *stmt = 0;
      const char *req = "INSERT INTO robots "
                        "(name, endpoint, connection_time, lastping_time) "
                        "VALUES (?, ?, datetime('now'), datetime('now'))";
      sqlite3_prepare_v2(_db, req, -1, &stmt, 0);
      sqlite3_bind_text(stmt, 1, url.str().c_str(), url.str().size(), 0);
      sqlite3_bind_text(stmt, 2, url.str().c_str(), url.str().size(), 0);

      if (sqlite3_step(stmt) != SQLITE_DONE)
      {
        qiLogError("lb") << "query failed: " << sqlite3_errmsg(_db);
      }
    }
  }

  void refresh()
  {
  }

  RemoteConnection *client(qi::RemoteGateway *remoteGateway)
  {
    return _clients[remoteGateway];
  }

private:
  qi::Url                                          _listenURL;
  qi::TransportServer                              _server;
  qi::Session                                      _session; // for the networkThread...
  std::map<qi::RemoteGateway*, RemoteConnection*>  _clients;
  sqlite3                                         *_db;
};

int main(int argc, char *argv[])
{
  qi::Application app(argc, argv);
  if (argc != 2)
  {
    std::cerr << "Usage: " << argv[0] << " ADDRESS" << std::endl;
    return 1;
  }
  else
  {
    LoadBalancer lb(argv[1]);
    lb.listen();
    app.run();
  }

  return 0;
}

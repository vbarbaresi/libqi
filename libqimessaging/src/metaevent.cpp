/*
**
** Copyright (C) 2012 Aldebaran Robotics
*/

#include "src/metaevent_p.hpp"
#include <qimessaging/metaevent.hpp>
#include <qimessaging/object.hpp>
namespace qi {

  MetaEvent::MetaEvent()
    : _p(new MetaEventPrivate())
  {
  }

  MetaEvent::MetaEvent(const std::string &sig)
    : _p(new MetaEventPrivate(sig))
  {
  }

  MetaEvent::MetaEvent(const MetaEvent &other)
    : _p(new MetaEventPrivate())
  {
    *_p = *(other._p);
  }

  MetaEvent& MetaEvent::operator=(const MetaEvent &other)
  {
    *_p = *(other._p);
    return (*this);
  }

  MetaEvent::~MetaEvent()
  {
    delete _p;
  }

  const std::string &MetaEvent::signature() const
  {
    return _p->signature();
  }

  unsigned int       MetaEvent::uid() const
  {
    return _p->uid();
  }

  static void functor_call2(MetaFunction f,
    const MetaFunctionParameters& args)
  {
    f(args);
  }

  void EventSubscriber::call(const MetaFunctionParameters& args)
  {
    if (handler)
    {
      if (eventLoop)
      {
        MetaFunctionParameters copy = args.copy();
        eventLoop->asyncCall(0,
          boost::bind(&functor_call2, handler, copy));
      }
      else
        handler(args);
    }
    if (target)
      target->metaEmit(method, args);
  }


  qi::ODataStream &operator<<(qi::ODataStream &stream, const MetaEvent &meta) {
    stream << meta._p->_signature;
    stream << meta._p->_uid;
    return stream;
  }

  qi::IDataStream &operator>>(qi::IDataStream &stream, MetaEvent &meta) {
    stream >> meta._p->_signature;
    stream >> meta._p->_uid;
    return stream;
  }

  qi::SignatureStream &operator&(qi::SignatureStream &stream, const MetaEvent &meta) {
    stream & meta._p->_signature;
    stream & meta._p->_uid;
    return stream;
  }

};

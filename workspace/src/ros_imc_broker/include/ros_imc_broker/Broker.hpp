//*************************************************************************
// Copyright (C) 2016 OceanScan - Marine Systems & Technology, Lda.       *
//*************************************************************************
// This program is free software; you can redistribute it and/or modify   *
// it under the terms of the GNU General Public License as published by   *
// the Free Software Foundation; either version 2 of the License, or (at  *
// your option) any later version.                                        *
//                                                                        *
// This program is distributed in the hope that it will be useful, but    *
// WITHOUT ANY WARRANTY; without even the implied warranty of             *
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
// General Public License for more details.                               *
//                                                                        *
// You should have received a copy of the GNU General Public License      *
// along with this program; if not, write to the Free Software            *
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA          *
// 02110-1301 USA.                                                        *
//*************************************************************************
//Author: Ricardo Martins                                                 *
//*************************************************************************

#ifndef ROS_IMC_BROKER_BROKER_HPP_INCLUDED_
#define ROS_IMC_BROKER_BROKER_HPP_INCLUDED_

// Boost headers.
#include <boost/thread/thread.hpp>
#include <boost/shared_ptr.hpp>

// ROS headers.
#include <ros/ros.h>
#include <dynamic_reconfigure/server.h>

// DUNE headers.
#include <DUNE/DUNE.hpp>

// Local headers.
#include <ros_imc_broker/Mappings.hpp>
#include <ros_imc_broker/TcpLink.hpp>
#include <ros_imc_broker/BrokerConfig.h>

namespace ros_imc_broker
{
  class Broker
  {
  public:
    Broker(ros::NodeHandle& node_handle):
      nh_(node_handle),
      tcp_client_(NULL),
      tcp_client_thread_(NULL)
    {
      srv_.setCallback(boost::bind(&Broker::onReconfigure, this, _1, _2));
      advertiseAll();
      subscribeAll();
    }

    ~Broker(void)
    {
      stop();
    }

  private:
    //! ROS node handle.
    ros::NodeHandle& nh_;
    //! TCP client to DUNE's server.
    TcpLink* tcp_client_;
    //! TCP client thread.
    boost::thread* tcp_client_thread_;
    //! Map of publishers.
    std::map<unsigned, ros::Publisher> pubs_;
    //! Map of subscribers by topic.
    std::map<std::string, ros::Subscriber> subs_;
    //! Dynamic reconfigure server.
    dynamic_reconfigure::Server<ros_imc_broker::BrokerConfig> srv_;

    void
    onReconfigure(ros_imc_broker::BrokerConfig& config, uint32_t level)
    {
      ROS_INFO("reconfigure request: %s %s",
               config.server_addr.c_str(),
               config.server_port.c_str());

      start(config.server_addr, config.server_port);
    }

    void
    stop(void)
    {
      if (tcp_client_thread_ == NULL)
        return;

      tcp_client_thread_->interrupt();
      tcp_client_thread_->join();
      delete tcp_client_thread_;
      tcp_client_thread_ = NULL;
      delete tcp_client_;
      tcp_client_ = NULL;
    }

    std::map<unsigned, ros::Publisher>::iterator
    advertise(const std::string& msg_name, unsigned msg_id)
    {
      std::map<unsigned, PublisherCreator>::const_iterator itr = publisher_creators.find(msg_id);
      if (itr == publisher_creators.end())
        throw DUNE::IMC::InvalidMessageId(msg_id);

      std::string topic("IMC/In/");
      topic.append(msg_name);

      return pubs_.insert(std::pair<unsigned, ros::Publisher>(msg_id, itr->second(nh_, topic, 1000, false))).first;
    }

    //! Publish an IMC message to the ROS message bus.
    //! @param[in] msg message instance.
    void
    sendToRosBus(const DUNE::IMC::Message* msg)
    {
      std::map<unsigned, ros::Publisher>::iterator itr = pubs_.find(msg->getId());
      if (itr == pubs_.end())
        return;

      std::map<unsigned, Publisher>::const_iterator titr = publisher_by_id.find(msg->getId());
      if (titr == publisher_by_id.end())
        return;

      titr->second(itr->second, msg);
    }

    //! Start TCP client.
    //! @param[in] addr server address.
    //! @param[in] port server port.
    void
    start(const std::string& addr, const std::string& port)
    {
      stop();
      tcp_client_ = new TcpLink(boost::bind(&Broker::sendToRosBus, this, _1));
      tcp_client_->setServer(addr, port);
      tcp_client_thread_ = new boost::thread(boost::ref(*tcp_client_));
    }

    //! Send message to TCP server.
    //! @param[in] msg message instance.
    template<typename T>
    void
    sendToTcpServer(const T& msg)
    {
      tcp_client_->write(&msg);
    }

    //! Subscribe to all IMC messages received over the ROS message bus.
    void
    advertiseAll(void)
    {
      std::vector<std::string> abbrevs;
      DUNE::IMC::Factory::getAbbrevs(abbrevs);

      std::vector<std::string>::const_iterator itr = abbrevs.begin();
      for (; itr != abbrevs.end(); ++itr)
        advertise(*itr, DUNE::IMC::Factory::getIdFromAbbrev(*itr));
    }

    //! Subscribe to all IMC messages received over the ROS message bus.
    void
    subscribeAll(void)
    {
      std::vector<std::string> abbrevs;
      DUNE::IMC::Factory::getAbbrevs(abbrevs);

      std::vector<std::string>::const_iterator itr = abbrevs.begin();
      for (; itr != abbrevs.end(); ++itr)
      {
        std::string topic = std::string("IMC/Out/") + *itr;

        if (false)
        {
          continue;
        }
#define MESSAGE(id, abbrev)                                             \
        else if (*itr == #abbrev)                                       \
        {                                                               \
          ROS_DEBUG("subscribing: %s", topic.c_str());                  \
          ros::Subscriber sub = nh_.subscribe(topic, 1000, &Broker::sendToTcpServer<DUNE::IMC::abbrev>, this); \
          subs_.insert(std::pair<std::string, ros::Subscriber>(topic, sub)); \
        }
#include <DUNE/IMC/Factory.def>
      }
    }
  };
}

#endif

#include <chrono>
#include <future>

#include <ros/connection.h>
#include <ros/service_manager.h>
#include <ros/service_server_link.h>

#include <foxglove_bridge/service_utils.hpp>

namespace foxglove_bridge {

std::string retrieveServiceType(const std::string& serviceName, std::chrono::milliseconds timeout) {
  auto link = ros::ServiceManager::instance()->createServiceServerLink(serviceName, false, "*", "*",
                                                                       {{"probe", "1"}});
  if (!link) {
    throw std::runtime_error("Failed to create service link");
  } else if (!link->getConnection()) {
    throw std::runtime_error("Failed to get service link connection");
  }

  std::promise<std::string> promise;
  auto future = promise.get_future();

  link->getConnection()->setHeaderReceivedCallback(
    [&promise](const ros::ConnectionPtr&, const ros::Header& header) {
      std::string serviceType;
      if (header.getValue("type", serviceType)) {
        promise.set_value(serviceType);
      } else {
        promise.set_exception(std::make_exception_ptr(
          std::runtime_error("Key 'type' not found in service connection header")));
      }
      return true;
    });

  if (future.wait_for(timeout) != std::future_status::ready) {
    throw std::runtime_error("Timed out when retrieving service type");
  }

  return future.get();
}

}  // namespace foxglove_bridge

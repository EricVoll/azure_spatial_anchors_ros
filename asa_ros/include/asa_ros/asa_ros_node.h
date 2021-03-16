#pragma once

#include <ros/ros.h>

#include <image_transport/image_transport.h>
#include <message_filters/subscriber.h>
#include <message_filters/time_synchronizer.h>
#include <message_filters/sync_policies/approximate_time.h>
#include <tf2_msgs/TFMessage.h>
#include <tf2_ros/buffer.h>
#include <tf2_ros/static_transform_broadcaster.h>
#include <tf2_ros/transform_broadcaster.h>
#include <tf2_ros/transform_listener.h>

#include <asa_ros_msgs/CreateAnchor.h>
#include <asa_ros_msgs/CreatedAnchor.h>
#include <asa_ros_msgs/FindAnchor.h>
#include <asa_ros_msgs/FoundAnchor.h>
#include <sensor_msgs/CameraInfo.h>
#include <sensor_msgs/Image.h>
#include <std_srvs/Empty.h>
#include <std_msgs/Empty.h>
#include <Eigen/Geometry>

#include "asa_ros/asa_interface.h"

namespace asa_ros {


typedef message_filters::sync_policies::ApproximateTime<sensor_msgs::Image,
                                            sensor_msgs::CameraInfo> CameraSyncPolicy;


class AsaRosNode {
 public:
  AsaRosNode(const ros::NodeHandle& nh, const ros::NodeHandle& nh_private);
  virtual ~AsaRosNode();

  void initFromRosParams();

 private:
  // Subscriber callbacks.
  void imageAndInfoCallback(const sensor_msgs::ImageConstPtr& image,
                            const sensor_msgs::CameraInfoConstPtr& camera_info);
  void imageCallback(const sensor_msgs::ImageConstPtr& image);
  void infoCallback(const sensor_msgs::CameraInfoConstPtr& camera_info);
  void transformCallback(const geometry_msgs::TransformStamped& msg);

  // ASA interface callbacks for publishing.
  void anchorFoundCallback(const std::string& anchor_id,
                           const Eigen::Affine3d& anchor_in_world_frame);
  void anchorCreatedCallback(bool success, const std::string& anchor_id,
                             const std::string& reason);

  // Service callbacks.
  bool createAnchorCallback(asa_ros_msgs::CreateAnchorRequest& req,
                            asa_ros_msgs::CreateAnchorResponse& res);
  bool findAnchorCallback(asa_ros_msgs::FindAnchorRequest& req,
                          asa_ros_msgs::FindAnchorResponse& res);
  bool resetCallback(std_srvs::EmptyRequest& req, std_srvs::EmptyResponse& res);
  bool resetCompletelyCallback(std_srvs::EmptyRequest& req,
                               std_srvs::EmptyResponse& res);

  // Wrappers for commonly-used calls to the interface.
  // Queries COMMA-SEPARATED list of anchor IDs, and caches them in case the
  // reset function/service is called. This means on calls to reset(), these
  // anchors will be automatically re-tracked.
  bool queryAnchors(const std::string& anchor_ids);

  // Timer callbacks.
  void createAnchorTimerCallback(const ros::TimerEvent& e);

  void AsaStatusChanged(float status);

  // Nodehandles, publishers, subscribers.
  ros::NodeHandle nh_;
  ros::NodeHandle nh_private_;

  // Sync camera and camera_info msgs.
  message_filters::Subscriber<sensor_msgs::Image> image_sub_;
  message_filters::Subscriber<sensor_msgs::CameraInfo> info_sub_;
  std::unique_ptr<message_filters::Synchronizer<CameraSyncPolicy>>
      image_info_approx_sync_;
      
  std::unique_ptr<message_filters::TimeSynchronizer<sensor_msgs::Image,
                                                    sensor_msgs::CameraInfo>>
      image_info_sync_; 

  // Pubs & subs
  ros::Publisher found_anchor_pub_;
  ros::Publisher created_anchor_pub_;
  ros::Publisher ready_to_operate_pub_;
  ros::Subscriber transform_sub_;
  ros::Subscriber nosync_image_sub_;
  ros::Subscriber nosync_camera_info_sub_;

  // Services.
  ros::ServiceServer create_anchor_srv_;
  ros::ServiceServer find_anchor_srv_;
  ros::ServiceServer reset_srv_;
  ros::ServiceServer reset_completely_srv_;

  // TF.
  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;
  tf2_ros::StaticTransformBroadcaster tf_broadcaster_;

  // ASA interface to actually set up the watching and creating anchors.
  std::unique_ptr<AzureSpatialAnchorsInterface> interface_;

  // Parameters
  std::string world_frame_id_;
  std::string camera_frame_id_;
  std::string anchor_frame_id_;

  // Timeout to wait for TF messages, in seconds. 0.0 = instantaneous. 1.0 =
  // will wait a full second on any failed attempt.
  double tf_lookup_timeout_;

  // A flag indicating that the node will use an approximate time synchronization  
  // policy to synchronize the images with the camera_info messages instead of the 
  // exact synchronizer
  bool use_approx_sync_policy;

  // The queue size of the subscribers used for the image and camera_info topic
  int queue_size;

  // If true, the node will publish an empty message to notify subscribers that the node
  // is not ready to query and create anchors
  bool should_publish_ready_msgs;

  // If true, then the first camera_info topic will be used for all images
  // This avoids having to synchronize images and infos
  bool should_not_synch_msgs;

  sensor_msgs::CameraInfo::ConstPtr first_info_msg;

  // Cache of which anchors are currently being queried. This will be only used
  // when reset() (but not resetCompletely() is called, to restart any
  // previous watchers.
  std::string anchor_ids_;
};

}  // namespace asa_ros

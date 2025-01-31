// ros
#include <cv_bridge/cv_bridge.h>
#include <image_transport/image_transport.h>
#include <ros/ros.h>

// flightlib
#include "flightlib/bridges/unity_bridge.hpp"
#include "flightlib/bridges/unity_message_types.hpp"
#include "flightlib/common/quad_state.hpp"
#include "flightlib/common/types.hpp"
#include "flightlib/objects/quadrotor.hpp"
#include "flightlib/sensors/rgb_camera.hpp"

using namespace flightlib;

std::shared_ptr<Quadrotor> initQuadrotor(std::shared_ptr<UnityBridge>& unity_bridge_ptr){
  std::shared_ptr<Quadrotor> quad_ptr = std::make_shared<Quadrotor>();

  // define quadsize scale (for unity visualization only)
  Vector<3> quad_size(0.5, 0.5, 0.5);
  quad_ptr->setSize(quad_size);
  QuadState quad_state;
  // initialization
  quad_state.setZero();
  quad_ptr->reset(quad_state);

  return quad_ptr;
}

int main(int argc, char *argv[]) {
  // initialize ROS
  ros::init(argc, argv, "camera_example");
  ros::NodeHandle nh("");
  ros::NodeHandle pnh("~");
  ros::Rate(50.0);

  // publisher
  image_transport::Publisher rgb_pub;
  image_transport::Publisher depth_pub;
  image_transport::Publisher segmentation_pub;
  image_transport::Publisher opticalflow_pub;

  image_transport::ImageTransport it(pnh);
  rgb_pub = it.advertise("/rgb", 1);
  depth_pub = it.advertise("/depth", 1);
  segmentation_pub = it.advertise("/segmentation", 1);
  opticalflow_pub = it.advertise("/opticalflow", 1);

  // unity quadrotor
  std::shared_ptr<UnityBridge> unity_bridge_ptr = UnityBridge::getInstance();
  SceneID scene_id{UnityScene::INDUSTRIAL};
  bool unity_ready{false};

  auto quad_ptr = initQuadrotor(unity_bridge_ptr);

  // Flightmare
  std::shared_ptr<RGBCamera> rgb_camera = std::make_shared<RGBCamera>();

  Vector<3> B_r_BC(0.0, -2.0, 0.0);// based on camera frame, i.e., z is front
  Matrix<3, 3> R_BC = Quaternion(1.0, 0.0, 0.0, 0.0).toRotationMatrix();
  std::cout << R_BC << std::endl;
  rgb_camera->setFOV(90);
  rgb_camera->setWidth(640);
  rgb_camera->setHeight(360);
  rgb_camera->setRelPose(B_r_BC, R_BC);
  rgb_camera->setPostProcesscing(
    std::vector<bool>{true, false, false});  // depth, segmentation, optical flow
  quad_ptr->addRGBCamera(rgb_camera);

  // start loop
  unity_bridge_ptr->addQuadrotor(quad_ptr);
  unity_ready = unity_bridge_ptr->connectUnity(scene_id);

  FrameID frame_id = 0;
  QuadState quad_state;
  quad_state.setZero();
  bool going_up = true;

  while (ros::ok() && unity_ready) {
    if (quad_state.x[QS::POSZ] > 5) going_up = false;
    else if (quad_state.x[QS::POSZ] < 0) going_up = true;
    
    //if (going_up) quad_state.x[QS::POSZ] += 0.1;
    //else quad_state.x[QS::POSZ] -= 0.1;

    quad_ptr->setState(quad_state);

    unity_bridge_ptr->getRender(frame_id);
    unity_bridge_ptr->handleOutput();
    frame_id += 1;

    cv::Mat img;
    ros::Time timestamp = ros::Time::now();
    
    rgb_camera->getRGBImage(img);
    sensor_msgs::ImagePtr rgb_msg =
      cv_bridge::CvImage(std_msgs::Header(), "bgr8", img).toImageMsg();
    rgb_msg->header.stamp = timestamp;
    rgb_pub.publish(rgb_msg);

    rgb_camera->getDepthMap(img);
    sensor_msgs::ImagePtr depth_msg =
      cv_bridge::CvImage(std_msgs::Header(), "32FC1", img).toImageMsg();
    depth_msg->header.stamp = timestamp;
    depth_pub.publish(depth_msg);

    rgb_camera->getSegmentation(img);
    sensor_msgs::ImagePtr segmentation_msg =
      cv_bridge::CvImage(std_msgs::Header(), "bgr8", img).toImageMsg();
    segmentation_msg->header.stamp = timestamp;
    segmentation_pub.publish(segmentation_msg);

    rgb_camera->getOpticalFlow(img);
    sensor_msgs::ImagePtr opticflow_msg =
      cv_bridge::CvImage(std_msgs::Header(), "bgr8", img).toImageMsg();
    opticflow_msg->header.stamp = timestamp;
    opticalflow_pub.publish(opticflow_msg);

  }

  return 0;
}
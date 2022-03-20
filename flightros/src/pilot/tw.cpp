#include "flightlib/bridges/unity_bridge.hpp"
#include "flightlib/common/quad_state.hpp"
#include "flightlib/common/types.hpp"
#include "flightlib/objects/quadrotor.hpp"
#include <unistd.h>

#include <Eigen/Dense>

#include "flightlib/objects/static_gate.hpp"
#include "flightlib/bridges/unity_message_types.hpp"

using namespace flightlib;

int main(){
    // Initialize quadrotor
    std::shared_ptr<Quadrotor> quad_ptr_ = std::make_shared<Quadrotor>();
    QuadState quad_state_;
    quad_state_.setZero();
    quad_ptr_->reset(quad_state_);

    // Initialize Unity bridge
    std::shared_ptr<UnityBridge> unity_bridge_ptr_;
    unity_bridge_ptr_ = UnityBridge::getInstance();

    // Add quadrotor
    unity_bridge_ptr_->addQuadrotor(quad_ptr_);
    bool unity_ready_ = unity_bridge_ptr_->connectUnity(UnityScene::INDUSTRIAL);

    // Define new quadrotor state
    quad_state_.x[QS::POSX] = (Scalar)0;
    quad_state_.x[QS::POSY] = (Scalar)0;
    quad_state_.x[QS::POSZ] = (Scalar)2;
    quad_state_.x[QS::ATTW] = (Scalar)0;
    quad_state_.x[QS::ATTX] = (Scalar)0;
    quad_state_.x[QS::ATTY] = (Scalar)0;
    quad_state_.x[QS::ATTZ] = (Scalar)1;

    // Set new state
    quad_ptr_->setState(quad_state_);

    // Render next frame
    unity_bridge_ptr_->getRender(0);
    unity_bridge_ptr_->handleOutput();


    // Initialize gates
    std::string object_id = "unity_gate"; // Unique name
    std::string prefab_id = "rpg_gate"; // Name of the prefab in the Assets/Resources folder
    std::shared_ptr<StaticGate> gate =
        std::make_shared<StaticGate>(object_id, prefab_id);
    gate->setPosition(Eigen::Vector3f(5, 0, 2.5));
    gate->setQuaternion(
        Quaternion(0, 0.0, 0.0, 1)
    );

    // // Initialize Unity bridge
    // std::shared_ptr<UnityBridge> unity_bridge_ptr_;
    // unity_bridge_ptr_ = UnityBridge::getInstance();

    // Add gates
    unity_bridge_ptr_->addStaticObject(gate);

     // Render next frame
    while(true){
        unity_bridge_ptr_->getRender(0);
        unity_bridge_ptr_->handleOutput();
    }
}


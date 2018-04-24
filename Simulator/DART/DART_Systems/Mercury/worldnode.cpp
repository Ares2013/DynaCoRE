#include "worldnode.hpp"
#include <Utils/utilities.hpp>

WorldNode::WorldNode(const dart::simulation::WorldPtr & world_,
        osgShadow::MinimalShadowMap * msm) :
    dart::gui::osg::WorldNode(world_, msm) {

        // setWolrd(world_);
        robot_ = world_->getSkeleton("mercury");

        mDof = robot_->getNumDofs();
        mQ = Eigen::VectorXd::Zero(mDof);
        mQdot = Eigen::VectorXd::Zero(mDof);
        Torques_ = Eigen::VectorXd::Zero(mDof);
        mTorqueCommand = Eigen::VectorXd::Zero(mDof);


        // initialize controller
        mController = dart::common::make_unique<Controller>(robot_);   
        //printf("The controller is constructed!!\n");

        // get initial pose of body
        Pelvis_ = robot_->getBodyNode("body");
        Pelvis_pos_init_ = Pelvis_->getTransform().translation();
        //Pelvis_pos_init_[2] = Pelvis_pos_init_[2] -0.1;

        rfoot_ = robot_->getBodyNode("rfoot");
        lfoot_ = robot_->getBodyNode("lfoot");
//rfoot_->setFrictionCoeff(5.0);
//lfoot_->setFrictionCoeff(5.0);
        printf("right friction: %f\n", rfoot_->getFrictionCoeff());
        printf("left friction: %f\n", lfoot_->getFrictionCoeff());

        // pelvis hold
        pelvis_hold =true;

        interface_ = new Mercury_interface();
        sensor_data_ = new Mercury_SensorData();
    }

WorldNode::~WorldNode() {
    delete interface_;
    delete sensor_data_;
}

void WorldNode::customPreStep() {
    // JPos Ctrl
    //_DART_JPosCtrl();

    // WBLC
    _WBDC_Ctrl();
    static int count(0);
    ++count;

    double curr_time = ((double)count)*(1.0/1500.);
    if(pelvis_hold == true) {
        if (curr_time < 1.5) { holdpelvis(); }
        //if (curr_time < 2000000.5) { holdpelvis(); }
        else if (curr_time < 2.8){
        //else if (curr_time < 200.8){
            holdhorizontal();
        } else {

        }
    } 

    Torques_ = robot_->getForces();
    //dynacore::pretty_print(mQ, std::cout, "dart Q");
    //dynacore::pretty_print(robot_->getGravityForces(), std::cout, "dart gravity");
    //dynacore::pretty_print(robot_->getForces(), std::cout, "dart forces");
    //dynacore::pretty_print(Torques_, std::cout, "dart torque");
}
void WorldNode::_WBDC_Ctrl(){
    mQ = robot_->getPositions();
    mQdot = robot_->getVelocities();
    mTorqueCommand.setZero();

    // Right Leg
    for(int i(0); i<3; ++i){
        sensor_data_->joint_jpos[i] = mQ[i+6];
        sensor_data_->motor_jpos[i] = mQ[i+6];
        sensor_data_->motor_jvel[i] = mQdot[i+6];
        sensor_data_->jtorque[i] = Torques_[i+6];
    }
    // Right Leg
    for(int i(0); i<3; ++i){
        sensor_data_->joint_jpos[i + 3] = mQ[i+10];
        sensor_data_->motor_jpos[i + 3] = mQ[i+10];
        sensor_data_->motor_jvel[i + 3] = mQdot[i+10];
        sensor_data_->jtorque[i + 3] = Torques_[i+10];
    }
    for(int i(0); i<3; ++i){
        // X, Y, Z

        // Rx, Ry, Rz
        sensor_data_->imu_ang_vel[i] = mQdot[i];
    }
    dynacore::Vect3 so3;
    for(int i(0); i<3; ++i) so3[i] = mQ[i];

    dynacore::Quaternion quat;
    dynacore::convert(so3, quat);
    
    // Foot contact
    dynacore::Vect3 rfoot_pos = rfoot_->getTransform().translation();
    dynacore::Vect3 lfoot_pos = lfoot_->getTransform().translation();

    double height_threshold(0.0484);
    if(rfoot_pos[2] < height_threshold)    sensor_data_->rfoot_contact = true;
    else sensor_data_->rfoot_contact = false;

    if(lfoot_pos[2] < height_threshold)    sensor_data_->lfoot_contact = true;
    else sensor_data_->lfoot_contact = false;


    static int count(0);
    ++count;
    //if(count % 2 == 0)  interface_->GetCommand(sensor_data_, cmd_);
    //dynacore::pretty_print(quat, std::cout, "dart quat");
    interface_->GetCommand(sensor_data_, cmd_);
    
    for(int i(0); i<3; ++i){
        mTorqueCommand[i + 6] = cmd_[i];
    }
    // Right ankle 
    double kp_ankle(0.1);
    double kd_ankle(0.01);
    mTorqueCommand[9] = kp_ankle * (-0.4 - mQ[9]) + kd_ankle * (0. - mQdot[9]);
    for(int i(0); i<3; ++i){
        mTorqueCommand[i + 10] = cmd_[i+3];
    }
    // Left ankle
    mTorqueCommand[13] = kp_ankle * (-0.4 - mQ[13]) + kd_ankle * (0. - mQdot[13]);
    //TEST
    //mTorqueCommand.setZero();
    
    robot_->setForces(mTorqueCommand);
}

void WorldNode::_DART_JPosCtrl(){
    // control force set zero
    mController->clearForces();

    // define the control gains
    mController->setPDGains();

    // define the desired joint position
    Eigen::VectorXd JPos_des(14);
    JPos_des[0] = 0.0;
    JPos_des[1] = 0.0;
    JPos_des[2] = 0.0;
    JPos_des[3] = 0.0;
    JPos_des[4] = 0.0;
    JPos_des[5] = 0.75;
    JPos_des[6] = 0.0;
    JPos_des[7] = -0.95;
    JPos_des[8] = 1.85;
    JPos_des[9] = 0.0;
    JPos_des[10] = 0.0;
    JPos_des[11] = -0.95;
    JPos_des[12] = 1.85;
    JPos_des[13] = 0.0;

    // compute the actuation force
    mController->setPDForces(JPos_des);

    // set actuation force to the joints
    setTorqueCommand(mController->Forces_);
}

void WorldNode::setTorqueCommand( Eigen::VectorXd& gamma){
    Eigen::VectorXd idx_joint(6);

    idx_joint[0] = robot_->getDof("1_torso_to_abduct_r_j")->getIndexInSkeleton();
    idx_joint[1] = robot_->getDof("abduct_r_to_thigh_r_j")->getIndexInSkeleton();
    idx_joint[2] = robot_->getDof("thigh_r_to_knee_r_j")->getIndexInSkeleton();
    idx_joint[3] = robot_->getDof("2_torso_to_abduct_l_j")->getIndexInSkeleton();
    idx_joint[4] = robot_->getDof("abduct_l_to_thigh_l_j")->getIndexInSkeleton();
    idx_joint[5] = robot_->getDof("thigh_l_to_knee_l_j")->getIndexInSkeleton();

    robot_->setForce((int)idx_joint[0], gamma[(int)idx_joint[0]]);
    robot_->setForce((int)idx_joint[1], gamma[(int)idx_joint[1]]);
    robot_->setForce((int)idx_joint[2], gamma[(int)idx_joint[2]]);

    robot_->setForce((int)idx_joint[3], gamma[(int)idx_joint[3]]);
    robot_->setForce((int)idx_joint[4], gamma[(int)idx_joint[4]]);
    robot_->setForce((int)idx_joint[5], gamma[(int)idx_joint[5]]);
}

void WorldNode::holdpelvis(){

    Eigen::Vector3d Pelvis_pos = Pelvis_->getTransform().translation();
    Eigen::Vector3d Pelvis_vel = Pelvis_->getLinearVelocity();
    //Eigen::Vector3d Pelvis_ori = Pelvis_->getTransform().eulerAngles(0,1,2);
    //Eigen::Vector3d Pelvis_ang_vel = Pelvis_->getAngularVelocity();


    Eigen::MatrixXd Kp = Eigen::MatrixXd::Identity(3,3);
    Eigen::MatrixXd Kd = Eigen::MatrixXd::Identity(3,3);

    // virtual joints
    for(int i(0); i<3 ;i++){
        Kp(i,i) = 1000.0;
        Kd(i,i) = 100.0;
    }

    Eigen::Vector3d Force;
    Force.setZero(); 

    Force = Kp*(Pelvis_pos_init_- Pelvis_pos) - Kd*Pelvis_vel;
    Force[2] += 20.*9.81;
    Pelvis_->addExtForce(Force);

    mQ = robot_->getPositions();
    mQdot = robot_->getVelocities();

    Eigen::Vector3d Torque;
    Torque = - Kp * mQ.head(3) - Kd*mQdot.head(3);
    Pelvis_->addExtTorque(Torque);
}

void WorldNode::holdhorizontal(){

    Eigen::Vector3d Pelvis_pos = Pelvis_->getTransform().translation();
    Eigen::Vector3d Pelvis_vel = Pelvis_->getLinearVelocity();
    //Eigen::Vector3d Pelvis_ori = Pelvis_->getTransform().eulerAngles(0,1,2);
    //Eigen::Vector3d Pelvis_ang_vel = Pelvis_->getAngularVelocity();

    Eigen::MatrixXd Kp = Eigen::MatrixXd::Identity(3,3);
    Eigen::MatrixXd Kd = Eigen::MatrixXd::Identity(3,3);

    // virtual joints
    for(int i(0); i<3 ;i++){
         //Kp(i,i) = 5.0;
        //Kd(i,i) = 0.3;
        Kp(i,i) = 1000.0;
        Kd(i,i) = 100.0;
    }

    Eigen::Vector3d Force;
    Force.setZero(); 

    Force = Kp*(Pelvis_pos_init_- Pelvis_pos) - Kd*Pelvis_vel;
    Force[2] = 0.;
    Pelvis_->addExtForce(Force);
}


#ifndef STATE_PROVIDER_DRACO_BIPED
#define STATE_PROVIDER_DRACO_BIPED

#include <Utils/utilities.hpp>
#include <Utils/wrap_eigen.hpp>
#include <Configuration.h>

using namespace dynacore;

class RobotSystem;

class DracoBip_StateProvider{
public:
  static DracoBip_StateProvider* getStateProvider();
  ~DracoBip_StateProvider(){}

  dynacore::Vect3 imu_ang_vel_;
  // Important!!!!!!!!
  int stance_foot_;

  double curr_time_;

  Vector Q_;
  Vector Qdot_;
  
  dynacore::Vect3 global_pos_local_;
  dynacore::Vect2 des_location_;

  int b_rfoot_contact_;
  int b_lfoot_contact_;
  int num_step_copy_;

private:
  DracoBip_StateProvider();
};


#endif

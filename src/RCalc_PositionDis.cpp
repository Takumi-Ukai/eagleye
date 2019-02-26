/*
 * RCalc_PositionDis.cpp
 * Position estimate program
 * Author Sekino
 * Ver 1.00 2019/02/26
 */

#include "ros/ros.h"
#include "geometry_msgs/Pose.h"
#include "geometry_msgs/PoseStamped.h"
#include "imu_gnss_localizer/VelocitySF.h"
#include "imu_gnss_localizer/Heading.h"
#include "imu_gnss_localizer/Distance.h"
#include "imu_gnss_localizer/UsrVel_enu.h"
#include "imu_gnss_localizer/Position.h"
#include <boost/circular_buffer.hpp>
#include <math.h>
#include <time.h>

ros::Publisher pub1;
ros::Publisher pub2;

bool flag_GNSS, flag_Est, flag_EstRaw, flag_Start, flag_Est_Raw_Heading;
int tmp1 = 0, tmp2 = 0, tmp3 = 0, tmp4 = 0, tmp5 = 0, tmp6 = 0, tmp7 = 0, tmp8 = 0, tmp9 = 0;
int i = 0;
int ESTNUM = 0;
int count = 0;
int GPS_count = 0;
int Distance_BUFNUM = 0;
int index_Dist = 0;
int seq = 0;
int seq_Last = 0;
int index_max = 0;
double IMUTime;
double IMUfrequency = 100; //IMU Hz
double IMUperiod = 1.0/IMUfrequency;
double ROSTime = 0.0;
double Time = 0.0;
double Time_Last = 0.0;
float sum_x, sum_y, sum_z;
float Correction_Velocity = 0.0;
float Distance_BUFNUM_MAX = 50000;//仮の値
float TH_VEL_EST = 10/3.6;
float ESTDIST = 500;
float TH_POSMAX = 3.0;
float TH_CALC_MINNUM = 1.0/20/2.5; //original 1.0/20 float TH_CALC_MINNUM = 1.0/20/2.5/2.0;
float TH_EST_GIVEUP_NUM = 1.0/100;
float UsrPos_enu_x = 0.0;
float UsrPos_enu_y = 0.0;
float UsrPos_enu_z = 0.0;
float UsrPos_EstRaw_enu_x = 0.0;
float UsrPos_EstRaw_enu_y = 0.0;
float UsrPos_EstRaw_enu_z = 0.0;
float UsrPos_Est_enu_x = 0.0;
float UsrPos_Est_enu_y = 0.0;
float UsrPos_Est_enu_z = 0.0;
float UsrVel_enu_E = 0.0;
float UsrVel_enu_N = 0.0;
float UsrVel_enu_U = 0.0;
float tUsrPos_enu_x = 0.0;
float tUsrPos_enu_y = 0.0;
float tUsrPos_enu_z = 0.0;
float UsrPos_Est_enu_x_Last = 0.0;
float UsrPos_Est_enu_y_Last = 0.0;
float UsrPos_Est_enu_z_Last = 0.0;
float Distance = 0.0;

std::size_t length_index;
std::size_t length_pflag_GNSS;
std::size_t length_pVelocity;
std::size_t length_pUsrPos_enu_x;
std::size_t length_pUsrPos_enu_y;
std::size_t length_pUsrPos_enu_z;
std::size_t length_pUsrVel_enu_E;
std::size_t length_pUsrVel_enu_N;
std::size_t length_pUsrVel_enu_U;
std::size_t length_pTime;
std::size_t length_pindex_vel;
std::size_t length_pindex_Raw;
std::size_t length_pDistance;
std::size_t length_index_Raw;

std::vector<bool> pflag_GNSS;
std::vector<float> pUsrPos_enu_x;
std::vector<float> pUsrPos_enu_y;
std::vector<float> pUsrPos_enu_z;
std::vector<float> pUsrVel_enu_E;
std::vector<float> pUsrVel_enu_N;
std::vector<float> pUsrVel_enu_U;
std::vector<float> pVelocity;
std::vector<double> pTime;
std::vector<float> basepos_x;
std::vector<float> basepos_y;
std::vector<float> basepos_z;
std::vector<float> pdiff2_x;
std::vector<float> pdiff2_y;
std::vector<float> pdiff2_z;
std::vector<float> basepos2_x;
std::vector<float> basepos2_y;
std::vector<float> basepos2_z;
std::vector<float> pdiff_x;
std::vector<float> pdiff_y;
std::vector<float> pdiff_z;
std::vector<float> pdiff;

boost::circular_buffer<float> pDistance(Distance_BUFNUM_MAX);
boost::circular_buffer<bool> pindex_Raw(Distance_BUFNUM_MAX);

std::vector<float>::iterator max;

geometry_msgs::Pose p1_msg;
imu_gnss_localizer::Position p2_msg;

void receive_VelocitySF(const imu_gnss_localizer::VelocitySF::ConstPtr& msg){

  Correction_Velocity = msg->Correction_Velocity;

}

void receive_Distance(const imu_gnss_localizer::Distance::ConstPtr& msg){

  Distance = msg->Distance;

}

void receive_Heading3rd(const imu_gnss_localizer::Heading::ConstPtr& msg){

  flag_Est_Raw_Heading = msg->flag_EstRaw;

}

void receive_UsrPos_enu(const geometry_msgs::PoseStamped::ConstPtr& msg){

  seq = msg->header.seq;
  UsrPos_enu_x = msg->pose.position.x; //unit [m]
  UsrPos_enu_y = msg->pose.position.y; //unit [m]
  UsrPos_enu_z = msg->pose.position.z; //unit [m]

}

void receive_UsrVel_enu(const imu_gnss_localizer::UsrVel_enu::ConstPtr& msg){

    //struct timespec startTime, endTime, sleepTime;

    ++count;

    IMUTime = IMUperiod * count;
    ROSTime = ros::Time::now().toSec();
    Time = ROSTime; //IMUTime or ROSTime
    //ROS_INFO("Time = %lf" , Time);

    if (0 == fmod(count,2)){ //50Hz用

    if (Distance_BUFNUM < Distance_BUFNUM_MAX){
      ++Distance_BUFNUM;
    }
    else{
      Distance_BUFNUM = Distance_BUFNUM_MAX;
    }

    //GNSS receive flag
    if (seq_Last == seq){
      flag_GNSS = false;
      UsrPos_enu_x = 0;
      UsrPos_enu_y = 0;
      UsrPos_enu_z = 0;
    }
    else{
      flag_GNSS = true;
      UsrPos_enu_x = UsrPos_enu_x;
      UsrPos_enu_y = UsrPos_enu_y;
      UsrPos_enu_z = UsrPos_enu_z;
      ++GPS_count;
    }

    UsrVel_enu_E = msg->VelE;
    UsrVel_enu_N = msg->VelN;
    UsrVel_enu_U = msg->VelU;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////Start

    //data buffer generate
    pDistance.push_back(Distance);
    pflag_GNSS.push_back(flag_GNSS);
    pindex_Raw.push_back(flag_Est_Raw_Heading);
    pUsrPos_enu_x.push_back(UsrPos_enu_x);
    pUsrPos_enu_y.push_back(UsrPos_enu_y);
    pUsrPos_enu_z.push_back(UsrPos_enu_z);
    pUsrVel_enu_E.push_back(UsrVel_enu_E);
    pUsrVel_enu_N.push_back(UsrVel_enu_N);
    pUsrVel_enu_U.push_back(UsrVel_enu_U);
    pVelocity.push_back(Correction_Velocity);
    pTime.push_back(Time);

    //dynamic array
    std::vector<int> pindex_Dist;
    std::vector<int> pindex_Est;
    std::vector<int> index_Raw;
    std::vector<int> pindex_GNSS;
    std::vector<int> pindex_vel;
    std::vector<int> index;

    //std::size_t length_pflag_GNSS = std::distance(pflag_GNSS.begin(), pflag_GNSS.end());
    length_pindex_Raw = std::distance(pindex_Raw.begin(), pindex_Raw.end());
    length_pDistance = std::distance(pDistance.begin(), pDistance.end());

    //The role of "find function" in MATLAB !Suspect!
    for(i = 0; i < length_pindex_Raw; i++){
      if(pindex_Raw[i] == true){
          index_Raw.push_back(i);
      }
    }

    length_index_Raw = std::distance(index_Raw.begin(), index_Raw.end());

    //The role of "find ( ,1) function" in MATLAB !Suspect!
    for(i = 0; i < Distance_BUFNUM; i++){
      if(pDistance[i] > pDistance[Distance_BUFNUM-1] - ESTDIST){
          pindex_Dist.push_back(i);
      }
    }

    index_Dist = pindex_Dist[0];

    ESTNUM = Distance_BUFNUM - index_Dist; //Offline => count - index_Dist
    //ROS_INFO("ESTNUM %d",ESTNUM);

    length_pflag_GNSS = std::distance(pflag_GNSS.begin(), pflag_GNSS.end());
    length_pVelocity = std::distance(pVelocity.begin(), pVelocity.end());
    length_pUsrPos_enu_x = std::distance(pUsrPos_enu_x.begin(), pUsrPos_enu_x.end());
    length_pUsrPos_enu_y = std::distance(pUsrPos_enu_y.begin(), pUsrPos_enu_y.end());
    length_pUsrPos_enu_z = std::distance(pUsrPos_enu_z.begin(), pUsrPos_enu_z.end());
    length_pUsrVel_enu_E = std::distance(pUsrVel_enu_E.begin(), pUsrVel_enu_E.end());
    length_pUsrVel_enu_N = std::distance(pUsrVel_enu_N.begin(), pUsrVel_enu_N.end());
    length_pUsrVel_enu_U = std::distance(pUsrVel_enu_U.begin(), pUsrVel_enu_U.end());
    length_pTime = std::distance(pTime.begin(), pTime.end());

    tmp1 = length_pflag_GNSS - ESTNUM;
    tmp2 = length_pVelocity - ESTNUM;
    tmp3 = length_pUsrPos_enu_x - ESTNUM;
    tmp4 = length_pUsrPos_enu_y - ESTNUM;
    tmp5 = length_pUsrPos_enu_z - ESTNUM;
    tmp6 = length_pUsrVel_enu_E - ESTNUM;
    tmp7 = length_pUsrVel_enu_N - ESTNUM;
    tmp8 = length_pUsrVel_enu_U - ESTNUM;
    tmp9 = length_pTime - ESTNUM;

    if(tmp1 > 0){
      pflag_GNSS.erase(pflag_GNSS.begin(),pflag_GNSS.begin()+abs(tmp1));
      }
    else if(tmp1 < 0){
      for(i = 0; i < abs(tmp1); i++){
      pflag_GNSS.insert(pflag_GNSS.begin(),false);
      }
    }

    if(tmp2 > 0){
      pVelocity.erase(pVelocity.begin(),pVelocity.begin()+abs(tmp2));
    }
    else if(tmp2 < 0){
      for(i = 0; i < abs(tmp2); i++){
      pVelocity.insert(pVelocity.begin(),0);
      }
    }

    if(tmp3 > 0){
      pUsrPos_enu_x.erase(pUsrPos_enu_x.begin(),pUsrPos_enu_x.begin()+abs(tmp3));
    }
    else if(tmp3 < 0){
      for(i = 0; i < abs(tmp3); i++){
      pUsrPos_enu_x.insert(pUsrPos_enu_x.begin(),0);
      }
    }

    if(tmp4 > 0){
      pUsrPos_enu_y.erase(pUsrPos_enu_y.begin(),pUsrPos_enu_y.begin()+abs(tmp4));
    }
    else if(tmp4 < 0){
      for(i = 0; i < abs(tmp4); i++){
      pUsrPos_enu_y.insert(pUsrPos_enu_y.begin(),0);
      }
    }

    if(tmp5 > 0){
      pUsrPos_enu_z.erase(pUsrPos_enu_z.begin(),pUsrPos_enu_z.begin()+abs(tmp5));
    }
    else if(tmp5 < 0){
      for(i = 0; i < abs(tmp5); i++){
      pUsrPos_enu_z.insert(pUsrPos_enu_z.begin(),0);
      }
    }

    if(tmp6 > 0){
      pUsrVel_enu_E.erase(pUsrVel_enu_E.begin(),pUsrVel_enu_E.begin()+abs(tmp6));
    }
    else if(tmp6 < 0){
      for(i = 0; i < abs(tmp6); i++){
      pUsrVel_enu_E.insert(pUsrVel_enu_E.begin(),0);
      }
    }

    if(tmp7 > 0){
      pUsrVel_enu_N.erase(pUsrVel_enu_N.begin(),pUsrVel_enu_N.begin()+abs(tmp7));
    }
    else if(tmp7 < 0){
      for(i = 0; i < abs(tmp7); i++){
      pUsrVel_enu_N.insert(pUsrVel_enu_N.begin(),0);
      }
    }

    if(tmp8 > 0){
      pUsrVel_enu_U.erase(pUsrVel_enu_U.begin(),pUsrVel_enu_U.begin()+abs(tmp8));
    }
    else if(tmp8 < 0){
      for(i = 0; i < abs(tmp8); i++){
      pUsrVel_enu_U.insert(pUsrVel_enu_U.begin(),0);
      }
    }

    if(tmp9 > 0){
      pTime.erase(pTime.begin(),pTime.begin()+abs(tmp9));
    }
    else if(tmp9 < 0){
      for(i = 0; i < abs(tmp9); i++){
      pTime.insert(pTime.begin(),0);
      }
    }

    //The role of "find function" in MATLAB !Suspect!
    for(i = 0; i < ESTNUM; i++){
      if(pflag_GNSS[i] == true){
          pindex_GNSS.push_back(i);
      }
    }
      for(i = 0; i < ESTNUM; i++){
      if(pVelocity[i] > TH_VEL_EST){
          pindex_vel.push_back(i);
      }
    }

    length_pindex_vel = std::distance(pindex_vel.begin(), pindex_vel.end());

    //The role of "intersect function" in MATLAB
      set_intersection(pindex_GNSS.begin(), pindex_GNSS.end()
                     , pindex_vel.begin(), pindex_vel.end()
                     , inserter(index, index.end()));

      length_index = std::distance(index.begin(), index.end());

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////End (increase = 0.0015, decrease = 0.0025 )

    if(length_index_Raw > 0){

      if(pDistance[Distance_BUFNUM-1] > ESTDIST && flag_GNSS == true && Correction_Velocity > TH_VEL_EST && index_Dist > index_Raw[0] && count > 1 && ESTNUM != Distance_BUFNUM_MAX && 0 == fmod(GPS_count,10.0 )){

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////Start

          if(length_index > length_pindex_vel * TH_CALC_MINNUM){

            std::vector<float> tTrajectory_x(ESTNUM,0);
            std::vector<float> tTrajectory_y(ESTNUM,0);
            std::vector<float> tTrajectory_z(ESTNUM,0);

            for(i = 0; i < ESTNUM; i++){
              if(i > 0){
              tTrajectory_x[i] = tTrajectory_x[i-1] + pUsrVel_enu_E[i] * (pTime[i]-pTime[i-1]);
              tTrajectory_y[i] = tTrajectory_y[i-1] + pUsrVel_enu_N[i] * (pTime[i]-pTime[i-1]);
              tTrajectory_z[i] = tTrajectory_z[i-1] + pUsrVel_enu_U[i] * (pTime[i]-pTime[i-1]);
              }
            }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////End (0.0005~0.0015)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////Start

            /*
            clock_gettime(CLOCK_REALTIME, &startTime);
            sleepTime.tv_sec = 0;
            sleepTime.tv_nsec = 123;
            */

            while(1){

                length_index = std::distance(index.begin(), index.end());

                basepos_x.clear();
                basepos_y.clear();
                basepos_z.clear();

                for(i = 0; i < ESTNUM; i++){
                  basepos_x.push_back(pUsrPos_enu_x[index[length_index-1]] - tTrajectory_x[index[length_index-1]] + tTrajectory_x[i]);
                  basepos_y.push_back(pUsrPos_enu_y[index[length_index-1]] - tTrajectory_y[index[length_index-1]] + tTrajectory_y[i]);
                  basepos_z.push_back(pUsrPos_enu_z[index[length_index-1]] - tTrajectory_z[index[length_index-1]] + tTrajectory_z[i]);
                }

                pdiff2_x.clear();
                pdiff2_y.clear();
                pdiff2_z.clear();

                for(i = 0; i < length_index; i++){
                  pdiff2_x.push_back(basepos_x[index[i]] - pUsrPos_enu_x[index[i]]);
                  pdiff2_y.push_back(basepos_y[index[i]] - pUsrPos_enu_y[index[i]]);
                  pdiff2_z.push_back(basepos_z[index[i]] - pUsrPos_enu_z[index[i]]);
                }

                sum_x = 0.0;
                sum_y = 0.0;
                sum_z = 0.0;

                for(i = 0; i < length_index; i++){
                  sum_x += pdiff2_x[i];
                  sum_y += pdiff2_y[i];
                  sum_z += pdiff2_z[i];
                }

                tUsrPos_enu_x = pUsrPos_enu_x[index[length_index-1]] - sum_x / length_index;
                tUsrPos_enu_y = pUsrPos_enu_y[index[length_index-1]] - sum_y / length_index;
                tUsrPos_enu_z = pUsrPos_enu_z[index[length_index-1]] - sum_z / length_index;

                basepos2_x.clear();
                basepos2_y.clear();
                basepos2_z.clear();

                for(i = 0; i < ESTNUM; i++){
                  basepos2_x.push_back(tUsrPos_enu_x - tTrajectory_x[index[length_index-1]] + tTrajectory_x[i]);
                  basepos2_y.push_back(tUsrPos_enu_y - tTrajectory_y[index[length_index-1]] + tTrajectory_y[i]);
                  basepos2_z.push_back(tUsrPos_enu_z - tTrajectory_z[index[length_index-1]] + tTrajectory_z[i]);
                }

                pdiff_x.clear();
                pdiff_y.clear();
                pdiff_z.clear();

                for(i = 0; i < length_index; i++){
                  pdiff_x.push_back(basepos2_x[index[i]] - pUsrPos_enu_x[index[i]]);
                  pdiff_y.push_back(basepos2_y[index[i]] - pUsrPos_enu_y[index[i]]);
                  pdiff_z.push_back(basepos2_z[index[i]] - pUsrPos_enu_z[index[i]]);
                }

                pdiff.clear();
                for(i = 0; i < length_index; i++){
                pdiff.push_back(sqrt((pdiff_x[i] * pdiff_x[i]) + (pdiff_y[i] * pdiff_y[i]) + (pdiff_z[i] * pdiff_z[i])));
                }

                max = std::max_element(pdiff.begin(), pdiff.end());
                index_max = std::distance(pdiff.begin(), max);

                if(pdiff[index_max] > TH_POSMAX){
                  index.erase(index.begin() + index_max);
                }
                else{
                  break;
                }

                length_index = std::distance(index.begin(), index.end());
                length_pindex_vel = std::distance(pindex_vel.begin(), pindex_vel.end());

                if(length_index < length_pindex_vel * TH_EST_GIVEUP_NUM){
                  break;
                }

            }

            length_index = std::distance(index.begin(), index.end());
            length_pindex_vel = std::distance(pindex_vel.begin(), pindex_vel.end());

/*
            clock_gettime(CLOCK_REALTIME, &endTime);

            if (endTime.tv_nsec < startTime.tv_nsec) {
              ROS_INFO("ProTime = %ld.%09ld ESTNUM = %d Flag = %d", endTime.tv_sec - startTime.tv_sec - 1
                      ,endTime.tv_nsec + 1000000000 - startTime.tv_nsec , ESTNUM, flag_EstRaw);
            }
            else {
              ROS_INFO("ProTime = %ld.%09ld ESTNUM = %d Flag = %d", endTime.tv_sec - startTime.tv_sec
                      ,endTime.tv_nsec - startTime.tv_nsec , ESTNUM, flag_EstRaw);
            }
*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////End (0.0015~0.2)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////Start

            if(length_index >= length_pindex_vel * TH_EST_GIVEUP_NUM){

              if(index[length_index-1] == ESTNUM){
                UsrPos_EstRaw_enu_x = tUsrPos_enu_x;
                UsrPos_EstRaw_enu_y = tUsrPos_enu_y;
                UsrPos_EstRaw_enu_z = tUsrPos_enu_z;
              }
              else{
                UsrPos_EstRaw_enu_x = tUsrPos_enu_x + (tTrajectory_x[ESTNUM-1] - tTrajectory_x[index[length_index-1]]);
                UsrPos_EstRaw_enu_y = tUsrPos_enu_y + (tTrajectory_y[ESTNUM-1] - tTrajectory_y[index[length_index-1]]);
                UsrPos_EstRaw_enu_z = tUsrPos_enu_z + (tTrajectory_z[ESTNUM-1] - tTrajectory_z[index[length_index-1]]);
              }
              flag_EstRaw = true;
              flag_Start = true;

            }
            else{
              UsrPos_EstRaw_enu_x = 0.0;
              UsrPos_EstRaw_enu_y = 0.0;
              UsrPos_EstRaw_enu_z = 0.0;
              flag_EstRaw = false;
            }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////End (0.0000005)

          }

        }
        else{
          UsrPos_EstRaw_enu_x = 0.0;
          UsrPos_EstRaw_enu_y = 0.0;
          UsrPos_EstRaw_enu_z = 0.0;
          flag_EstRaw = false;
        }

      }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////Start

    if(flag_EstRaw == true){
      UsrPos_Est_enu_x = UsrPos_EstRaw_enu_x;
      UsrPos_Est_enu_y = UsrPos_EstRaw_enu_y;
      UsrPos_Est_enu_z = UsrPos_EstRaw_enu_z;
    }
    else if(count > 1){
      UsrPos_Est_enu_x = UsrPos_Est_enu_x_Last + UsrVel_enu_E * ( Time - Time_Last );
      UsrPos_Est_enu_y = UsrPos_Est_enu_y_Last + UsrVel_enu_N * ( Time - Time_Last );
      UsrPos_Est_enu_z = UsrPos_Est_enu_z_Last + UsrVel_enu_U * ( Time - Time_Last );
    }

    if(flag_Start == true){
      flag_Est = true;
    }
    else{
      flag_Est = false;
      UsrPos_Est_enu_x = 0.0;
      UsrPos_Est_enu_y = 0.0;
      UsrPos_Est_enu_z = 0.0;
    }

    p1_msg.position.x = UsrPos_Est_enu_x;
    p1_msg.position.y = UsrPos_Est_enu_y;
    p1_msg.position.z = UsrPos_Est_enu_z;

    p2_msg.enu_x = UsrPos_Est_enu_x;
    p2_msg.enu_y = UsrPos_Est_enu_y;
    p2_msg.enu_z = UsrPos_Est_enu_z;
    p2_msg.flag_Est = flag_Est;
    p2_msg.flag_EstRaw = flag_EstRaw;

    pub1.publish(p1_msg);
    pub2.publish(p2_msg);

    seq_Last = seq;
    Time_Last = Time;
    UsrPos_Est_enu_x_Last = UsrPos_Est_enu_x;
    UsrPos_Est_enu_y_Last = UsrPos_Est_enu_y;
    UsrPos_Est_enu_z_Last = UsrPos_Est_enu_z;

    ROS_INFO("ESTNUM = %d index_Dist %d ",ESTNUM,index_Dist);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////End (noFlag = 0.00003,Flag = 0.000009)

  } //50Hz用
}

int main(int argc, char **argv){

  ros::init(argc, argv, "RCalc_PositionDis");

  ros::NodeHandle n;
  ros::Subscriber sub1 = n.subscribe("/imu_gnss_localizer/UsrVel_enu", 1000, receive_UsrVel_enu);
  ros::Subscriber sub2 = n.subscribe("/gnss_pose", 1000, receive_UsrPos_enu);
  ros::Subscriber sub3 = n.subscribe("/imu_gnss_localizer/VelocitySF", 1000, receive_VelocitySF);
  ros::Subscriber sub4 = n.subscribe("/imu_gnss_localizer/Distance", 1000, receive_Distance);
  ros::Subscriber sub5 = n.subscribe("/imu_gnss_localizer/Heading3rd", 1000, receive_Heading3rd);
  pub1 = n.advertise<geometry_msgs::Pose>("/imu_gnss_pose", 1000);
  pub2 = n.advertise<imu_gnss_localizer::Position>("/imu_gnss_localizer/PositionDis", 1000);
  ros::spin();

  return 0;
}
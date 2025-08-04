#include <cstdio>
#include <fstream>
#include <iostream>
using namespace std;
struct IMUData {
    char type;
    double time;
    int week;
    int leap;
    double temperature;
    double gyro[3];
    double accel[3];
    int stamp;
    int delay;
};
IMUData imu_data{0};
int main()
{
    ifstream fin("/home/supremelyre/data/projects/GREAT-MSF/sample_data/MSF_20250318/IMU/2025_03_18_03_22_01.imu",
                 ios::in);
    ofstream fout("/home/supremelyre/data/projects/GREAT-MSF/sample_data/MSF_20250318/IMU/ADIS.txt", ios::out);
    char line0[1024];
    sprintf(line0, "%15s%15s%15s%15s%15s%15s%15s\n", "#Time", "Gyro_X", "Gyro_Y", "Gyro_Z", "Accel_X", "Accel_Y",
            "Accel_Z");
    fout << line0;
    while(!fin.eof())
    {
        fin.getline(line0,1024);
        // cout << line0;
        sscanf(line0, "%c,%lf,%d,%d,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%d,%d",&imu_data.type,&imu_data.time,&imu_data.week,&imu_data.leap,&imu_data.temperature,&imu_data.accel[0],&imu_data.accel[1],&imu_data.accel[2],&imu_data.gyro[0],&imu_data.gyro[1],&imu_data.gyro[2],&imu_data.stamp,&imu_data.delay);
        for(auto &i:imu_data.accel)
        {
            i *= 9.80665;
        }
        sprintf(line0,"%15.5lf%15.6lf%15.6lf%15.6lf%15.6lf%15.6lf%15.6lf\n",imu_data.time,imu_data.gyro[0],imu_data.gyro[1],imu_data.gyro[2],imu_data.accel[0],imu_data.accel[1],imu_data.accel[2]);
        fout << line0;
    }
    return 0;
}
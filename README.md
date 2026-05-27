### 建图
### 方法一： slamtoolbox方法
sudo apt update
sudo apt install ros-humble-slam-toolbox ros-humble-nav2-map-server
由于slam_toolbox需要
/scan -> 2D激光
/odom -> 轮式里程计
/tf -> base_link↔laser、odom↔base_link 已发布

终端1 启动机器人硬件（激光+odom+URDF）
ros2 launch fishbot_bringup bringup.launch.py
终端2 建图
ros2 launch fishbot_navigation2 slam_toolbox.launch.py use_sim_time:=False
终端3 键盘遥控
ros2 run teleop_twist_keyboard teleop_twist_keyboard
终端4 保存地图 
cd ~/my_maps
ros2 run nav2_map_server map_saver_cli -t map -f my_map

### 方法二： 建图Cartographer 方法
sudo apt install ros-$ROS_DISTRO-cartographer-ros
ros2 launch cartographer_ros cartographer_2d.launch.py use_sim_time:=false

### 导航
### 终端1 启动机器人硬件（激光+odom+URDF）
ros2 launch fishbot_bringup bringup.launch.py
### 终端2  定位加导航
ros2 launch fishbot_navigation2 navigation2.launch.py \ map:=/home/cat/fishbot_ws/src/fishbot_navigation2/maps/room_new.yaml \ use_sim_time:=false \ params_file:=/home/cat/fishbot_ws/src/fishbot_navigation2/config/nav2_params.yaml 

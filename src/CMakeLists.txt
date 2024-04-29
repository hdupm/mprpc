#aux_source_directory(. SRC_LIST) #有新增文件需要清空build再编译
set(SRC_LIST mprpcapplication.cc mprpcconfig.cc rpcheader.pb.cc rpcprovider.cc mprpcchannel.cc mprpccontroller.cc logger.cc zookeeperutil.cc)
add_library(mprpc ${SRC_LIST}) #如果里面是SHARED则是动态库 由于我们的muduo库是静态库 所以统一编成了静态库
target_link_libraries(mprpc muduo_net muduo_base pthread zookeeper_mt)#静态库顺序注意 先连接net mt是多线程版本
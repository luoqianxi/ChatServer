
#定义了一个SRC_LIST变量，用于存储当前目录下的所有源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./redis REDIS_LIST)

link_directories(/usr/local/lib)
#指定生成可生成文件
add_executable(ChatServer ${SRC_LIST} ${DB_LIST} ${MODEL_LIST} ${REDIS_LIST})
#链接所需的库
target_link_libraries(ChatServer muduo_net muduo_base mysqlclient hiredis pthread)


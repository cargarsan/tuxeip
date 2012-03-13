
set(CMAKE_EXECUTABLE_SUFFIX .cgi)

ADD_Executable(HistoChart histochart.c )

TARGET_LINK_LIBRARIES(HistoChart gd cgic sutil mysqlclient)



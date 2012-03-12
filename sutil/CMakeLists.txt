PROJECT(sutil)

ADD_LIBRARY(sutil STATIC src/ChainList.c src/iniparser.c src/List.c src/Misc.c src/MySQL.c)

#TARGET_LINK_LIBRARIES(mysql)


PROJECT(tuxeip)

ADD_LIBRARY(tuxeip STATIC src/AB.c
			  src/CIP_IOI.c
			  src/CM.c
			  src/ErrCodes.c
			  src/Ethernet_IP.c
			  src/LGX.c
			  src/MR.c
			  src/PLC.c
			  src/SendData.c)


#TARGET_LINK_LIBRARIES(mysql)


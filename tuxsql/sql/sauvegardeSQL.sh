#!/bin/sh
mysqldump histosql DEFINITION > histosql.sql
mysqldump histosql PLC >> histosql.sql
mysqldump histosql -d HISTO >> histosql.sql

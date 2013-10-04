/*
 * aggregate_bcon.cpp
 *
 *  Created on: Sep 11, 2013
 *      Author: Charlie
 */

#include <mongo.h>
#include <bcon.h>
#include <stdio.h>

int main() {
	/*
	 * We assume objects in the form of {_id:<any_id>, list:[{a:<int>,b:<int>}, ...]}
	 */
	char table[] = "agg";
	mongo conn[1];
	mongo_init(conn);
	if(MONGO_OK != mongo_client(conn, "127.0.0.1", 27017))
		return 1;
	bson b[1], b_result[1];

	/*create the aggregation command in bson*/
	bcon cmd_aggregate[] = { "aggregate", BRS(table),
			"pipeline",
			"[",
				"{",
					"$unwind", "$list",
				"}",
				"{",
					"$group",
					"{",
						"_id", "$list",
						"distinct_count",
						"{",
							"$sum", BI(1),
						"}",
					"}",
				"}",
			"]",
			BEND
	};
	bson_from_bcon(b, cmd_aggregate);

	/*So you can see your command*/
	bson_print(b);

	/*run the command*/
	mongo_run_command(conn, "test", b, b_result);

	/*command results*/
	bson_print(b_result);

	bson_destroy(b_result);
	bson_destroy(b);
	mongo_destroy(conn);
	return 0;
}

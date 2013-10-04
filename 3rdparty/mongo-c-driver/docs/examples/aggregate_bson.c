/*
 * aggregate_bson.cpp
 *
 *  Created on: Sep 11, 2013
 *      Author: Charlie
 */

#include <mongo.h>
#include <stdio.h>

int main() {
	/*
	 * We assume objects in the form of {_id:<any_id>, list:[{a:<int>,b:<int>}, ...]}
	 */
	mongo conn[1];
	mongo_init(conn);
	if(MONGO_OK != mongo_client(conn, "127.0.0.1", 27017))
		return 1;
	bson b[1], b_result[1];
	/*create the aggregation command in bson*/
	bson_init(b);
		bson_append_string(b, "aggregate", "agg");
		bson_append_start_array(b, "pipeline");
			bson_append_start_object(b,"0");
				bson_append_string(b, "$unwind", "$list");
			bson_append_finish_object(b);
			bson_append_start_object(b,"1");
				bson_append_start_object(b,"$group");
					bson_append_string(b,"_id", "$list");
					bson_append_start_object(b, "distinct_count");
						bson_append_int(b, "$sum", 1);
					bson_append_finish_object(b);
				bson_append_finish_object(b);
			bson_append_finish_object(b);
		bson_append_finish_array(b);
	bson_finish(b);

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


#include "markers.h"
#include "json.h"

using namespace cv;

namespace manus {

MarkersApiHandler::MarkersApiHandler() {


}

MarkersApiHandler::~MarkersApiHandler() {



}

#define STR2FLOAT(S) (atof((S).c_str()))
#define STR2INT(S) (atoi((S).c_str()))

void MarkersApiHandler::handle(Request& request) {

    if (!request.has_variable("command")) {
        request.set_status(404);
        request.send_data("Not found");
        request.finish();
        return;
    }

    string command = request.get_variable("command");
    Json::Value response;

	if (command == "clear") {

		markers.clear();

		response["count"] = (int) markers.size();

        request.set_status(200);
        request.set_header("Content-Type", "application/json");
        request.set_header("Cache-Control", "max-age=0, post-check=0, pre-check=0, no-store, no-cache, must-revalidate");
        Json::FastWriter writer;
        request.send_data(writer.write(response));


    } else if (command == "remove") {

		int id = atoi(request.get_variable("id").c_str());
		map<int, MarkerInfo>::iterator it = markers.find(id);

		if (it != markers.end())
			markers.erase(it);

		response["count"] = (int) markers.size();

        request.set_status(200);
        request.set_header("Content-Type", "application/json");
        request.set_header("Cache-Control", "max-age=0, post-check=0, pre-check=0, no-store, no-cache, must-revalidate");
        Json::FastWriter writer;
        request.send_data(writer.write(response));


    } else if (command == "put") {

		string sid = request.get_variable("id");

		int id = sid.empty() ? (counter++) : atoi(sid.c_str());

		Point3f position(STR2FLOAT(request.get_variable("x")), STR2FLOAT(request.get_variable("y")), STR2FLOAT(request.get_variable("z")));
		Point3f orientation(STR2FLOAT(request.get_variable("rx")), STR2FLOAT(request.get_variable("ry")), STR2FLOAT(request.get_variable("rz")));
		Scalar color(STR2INT(request.get_variable("red")), STR2INT(request.get_variable("green")), STR2INT(request.get_variable("blue")));

		MarkerInfo marker;
		marker.name = request.get_variable("name");
		marker.position = position;
		marker.orientation = orientation;
		marker.color = color;

		markers[id] = marker;

		response["id"] = id;
		response["count"] = (int) markers.size();

        request.set_status(200);
        request.set_header("Content-Type", "application/json");
        request.set_header("Cache-Control", "max-age=0, post-check=0, pre-check=0, no-store, no-cache, must-revalidate");
        Json::FastWriter writer;
        request.send_data(writer.write(response));


    } else if (command == "get") {

		int i = 0;
		for (map<int, MarkerInfo>::iterator it = markers.begin(); it != markers.end(); it++) {

		    Json::Value jpos;
		    jpos[0] = Json::Value(it->second.position.x);
		    jpos[1] = Json::Value(it->second.position.y);
		    jpos[2] = Json::Value(it->second.position.z);

		    Json::Value jori;
		    jori[0] = Json::Value(it->second.orientation.x);
		    jori[1] = Json::Value(it->second.orientation.y);
		    jori[2] = Json::Value(it->second.orientation.z);

		    Json::Value jcolor;
		    jcolor[0] = Json::Value(it->second.color[0]);
		    jcolor[1] = Json::Value(it->second.color[1]);
		    jcolor[2] = Json::Value(it->second.color[2]);

			Json::Value jmarker;
		    jmarker["id"] = Json::Value(it->first);
		    jmarker["name"] = Json::Value(it->second.name);
	  		jmarker["position"] = jpos;
		    jmarker["orientation"] = jori;
		    jmarker["color"] = jcolor;
			jmarker["id"] = Json::Value(it->first);

			response[i++] = jmarker;
		}

        request.set_status(200);
        request.set_header("Content-Type", "application/json");
        request.set_header("Cache-Control", "max-age=0, post-check=0, pre-check=0, no-store, no-cache, must-revalidate");
        Json::FastWriter writer;
        request.send_data(writer.write(response));


    } else {
        request.set_status(404);
        request.send_data("Not found");
    }



    request.finish();


}


}


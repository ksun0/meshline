#include <tytherm.h>
#include <map>

bool serverStarted = false;
HttpServer server;

void onIndex(HttpRequest& request, HttpResponse& response)
{
	response.setCache(86400, true); // It's important to use cache for better performance.
    response.redirect("http://" + WifiAccessPoint.getIP().toString() + "login.html");
}

void onSend(HttpRequest& request, HttpResponse& response)
{
	if(request.method == HTTP_POST) {
				debugf("Updating DB");
		// Update config
		if (request.getPostParameter("key") == NULL || request.getPostParameter("value") == NULL) {
					debugf("NULL postparams");
            response.setAllowCrossDomainOrigin("*");
            return;
		} else {
		    debugf("key: %s, value: %s", request.getPostParameter("key").c_str(), request.getPostParameter("value").c_str());
            db[request.getPostParameter("key")] = request.getPostParameter("value");
		}
	}
    response.setAllowCrossDomainOrigin("*");
}

void onSync(HttpRequest& request, HttpResponse& response)
{
	if(request.method == HTTP_POST) {
				debugf("Updating DB");
		// Update config
		if (request.getBody() == NULL) {
					debugf("NULL params");
		} else {
			DynamicJsonBuffer buffer;

			JsonObject& root = buffer.parseObject(request.getBody().c_str());

			for (auto kv : root) {
				db[String(kv.key)] = String(kv.value.asString());
			}
		}

	}

    JsonObjectStream* stream = new JsonObjectStream();
    JsonObject& json = stream->getRoot();

    for ( unsigned int i = 0; i < db.count(); i++){
        json[db.keyAt(i)] = db.valueAt(i);
    };

    response.setAllowCrossDomainOrigin("*");
    response.sendDataStream(stream, MIME_JSON);
}

void onFile(HttpRequest& request, HttpResponse& response)
{
	Serial.println("got file req");
	String file = request.uri.Path;
	if(file[0] == '/')
		file = file.substring(1);

	if(file[0] == '.')
		response.code = HTTP_STATUS_FORBIDDEN;
	else {
		response.setCache(86400, true); // It's important to use cache for better performance.
		if (!response.sendFile(file)) {
		    response.redirect("http://" + WifiAccessPoint.getIP().toString() + "login.html");
		}
	}
}

void onGetState(HttpRequest& request, HttpResponse& response)
{
	JsonObjectStream* stream = new JsonObjectStream();
	JsonObject& json = stream->getRoot();

    for ( unsigned int i = 0; i < db.count(); i++){
		json[db.keyAt(i)] = db.valueAt(i);
    };

    response.setAllowCrossDomainOrigin("*");

    response.sendDataStream(stream, MIME_JSON);
}

void startWebServer()
{
	if(serverStarted)
		return;

	server.listen(80);
	server.addPath("/", onIndex);
    server.addPath("/send", onSend);
    server.addPath("/sync", onSync);
	server.addPath("/state.json", onGetState);
	server.setDefaultHandler(onFile);
	serverStarted = true;

	Serial.println("Starting server");

	if(WifiStation.isEnabled())
		debugf("STA: %s", WifiStation.getIP().toString().c_str());
	if(WifiAccessPoint.isEnabled())
		debugf("AP: %s", WifiAccessPoint.getIP().toString().c_str());
}

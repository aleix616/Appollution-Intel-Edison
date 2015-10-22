#include <stdio.h>
#include <parse.h>

#define ApplicationID "NkLzqvxNk1AA4Pe3OF9Wz9s50SPFrACHsCrICF7z"
#define ClientKey "hspT4otUy5qNvKoSk7QPNrLTMCfTvAfWSPx1IESY"

char* ObjectId[50] = "";

void sendDataToCloud(ParseClient client){
	char* request[200];
	strcpy("/1/classes/Sensors/");
	strcat(request, ObjectId);
	parseSendRequest(client, "PUT", request, "{\"foo\":\"bar\"}", NULL);
}

void myPushCallback(ParseClient client, int error, const char *buffer) {
  if (error == 0 && buffer != NULL) {
  	if (buffer == "Update") sendDataToCloud(client);
    printf("received push: '%s'\n", buffer);
  }
}

int main(int argc, char *argv[]) {

	ParseClient client = parseInitialize(ApplicationID, ClientKey);
	sendDataToCloud(client);

	parseSetPushCallback(client, myPushCallback);
	parseStartPushService(client);
	parseRunPushLoop(client);

    return 0;
}
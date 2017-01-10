#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <EthernetUdp.h>
#include <math.h>
#include <string.h>

//Define Mac Adress
byte mac[] = {
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02
};

//Initialize EthernetServer Webserver on port 80
EthernetServer server(80);

void setup() { 
  //Open serial communications and wait for port to open
  Serial.begin(9600);
  while (!Serial) {
    ;
  }
  
  //Start Internet Connection and check whether Ethernet was succesfully configured with DHCP
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    //No point in carrying on, so do nothing forevermore
    for (;;)
      ;
  }
  //Start the server and display IP Address
  server.begin();
  Serial.print("Server is at ");
  Serial.println(Ethernet.localIP());
}

void loop() {
  //Renew DHCP lease, show status notifications on serial monitor
  switch (Ethernet.maintain())
  {
    case 1:
      //renewed fail
      Serial.println("Error: renewed fail");
      break;

    case 2:
      //renewed success
      Serial.println("Renewed success");

      //print your local IP address:
      printIPAddress();
      break;

    case 3:
      //rebind fail
      Serial.println("Error: rebind fail");
      break;

    case 4:
      //rebind success
      Serial.println("Rebind success");

      //print your local IP address:
      printIPAddress();
      break;

    default:
      //nothing happened
      break;
  }
  
  //Listen for incoming clients and execute code in if statement when client connects
  EthernetClient client = server.available();
  if (client) {
    Serial.println("New client");
    //Define and/or Initialize used variables
    boolean currentLineIsBlank = true;
    boolean inRequestBody = false;
    boolean endOfRequest = false;
    int RequestContentLengthArray[3] = {-1, -1, -1};
    int RequestContentLength = 0;
    int RequestContentLengthIntCount = 0;
    String RequestMethod = "";
    String RequestHeaderLine = "";
    String RequestBody = "";
    String ResponseString = "";
    
    //Execute code in while-loop as long as the connection is open
    while (client.connected()) {
      if (client.available()) {        

        //Get next character of HTTP Request
        char c = client.read();
        //Empty RequestHeaderLine when starting on a new line of the HTTP Request
        if(currentLineIsBlank) {
          RequestHeaderLine = "";
        }

        //Fill RequestHeaderLine with all characters on the current line of the HTTP Request as long as Content-Length parameter isn't found
        if(RequestHeaderLine != "Content-Length: ") {
          //Construct the RequestContentLength integer when the RequestContentLengthArray has been filled;
          //RequestContentLength specifies the length of the HTTP Request Body
          if(RequestContentLength == 0 && RequestContentLengthIntCount > 0) {
            RequestContentLengthIntCount -= 1; //Compensate for extra RequestContentLengthIntCount++ when adding the last number in the below else-if block
            for(int i = 1; i < (RequestContentLengthIntCount + 2); i++) {
            RequestContentLength += (RequestContentLengthArray[(i-1)] * pow(10, ((RequestContentLengthIntCount + 1) - i)));
            }
          }
          else if((RequestHeaderLine == "POST" || RequestHeaderLine == "GET") && !inRequestBody) {
            RequestMethod = RequestHeaderLine;
          }
          RequestHeaderLine += c;
        } else if(isdigit(c)){
          //Fill Array with numbers for the Content-Length parameter in the HTTP Request header  
          RequestContentLengthArray[RequestContentLengthIntCount] = (c - '0'); //Convert character c to int
          RequestContentLengthIntCount++;
        }
        
        //Put the contents of the HTTP request body in the RequestBody variable
        if(inRequestBody) {
          RequestBody += c;
        }

        //Indicate the HTTP request is finished when all characters from the request body have been read
        if(RequestBody.length() == RequestContentLength && RequestContentLength != 0) {
          inRequestBody = false;
          endOfRequest = true;
        }

        //Indicate the HTTP request body has been reached when encountering an empty line after the HTTP header
        if (currentLineIsBlank && c == '\n') {
          inRequestBody = true;
        }

        //Set currentLineIsBlank to indicate whether we're at the start of a new line or not
        if (c == '\n') {
          //We're on the start of a new line
          currentLineIsBlank = true;
        } else if(c != '\r') {
          //C is a character, so we haven't reached a new line.
          currentLineIsBlank = false;
        }
        
        //Execute code in the if-statement when the request is finished
        if (endOfRequest) {
          Serial.print(RequestMethod+ ' ');
          Serial.println(RequestBody);
          ResponseString = generateResponse(RequestBody, RequestMethod);
          //Send the HTTP response
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html; charset=utf-8");
          client.println();
          client.println(ResponseString);
          break;
        }
      }
    }
    //Give the web browser time to receive the data
    delay(1);
    //Close the connection:
    client.stop();
    Serial.println("client disconnected");
  }
}

void printIPAddress()
{
  Serial.print("Server is at ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
  }

  Serial.println();
}

String generateResponse(String RequestBody, String RequestMethod)
{
  //Define and/or Initialize used variables
  bool inId = true;
  int IdActionPairCount = 0;
  String ResponseBody = "";
  String Id = "";
  String Action = "";
  
  //Determine the amount of Id/Action pairs
  for(int i = 0; i < RequestBody.length(); i++) {
    if(RequestBody[i] == '='){
      IdActionPairCount++;
    }
  };
  //Create Arrays for Id's and Actions and reset IdActionPairCount for re-use in next for-loop
  char Ids[IdActionPairCount][20];
  char Actions[IdActionPairCount][20];

  IdActionPairCount = 0;

  //Fill Arrays with Id's and Actions;
  //Corresponding Id's and their Actions will be at the same index in both arrays
  for(int i = 0; i < RequestBody.length(); i++) {
    if(RequestBody[i] == '&') {
      Action.toCharArray(Actions[IdActionPairCount], (Action.length()+1));
      Action = "";
      IdActionPairCount++;
      inId = true;
    } else if(RequestBody[i] == '=') {
      Id.toCharArray(Ids[IdActionPairCount], (Id.length()+1));
      Id = "";
      inId = false;
    } else if(inId) {
      Id += RequestBody[i];
    } else {
      Action += RequestBody[i];
    }
  }
  //Push last Action to Actions array and update variables for the last time
  Action.toCharArray(Actions[IdActionPairCount], (Action.length()+1));
  Action = "";
  IdActionPairCount++;
  inId = true;

  //Execute logic using id's and actions and generate the HTTP Response body as a string
  if(RequestMethod == "POST") {

  }
  if(RequestMethod == "GET") {
    
  }
  return ResponseBody; //Return string to be sent back to the client
}


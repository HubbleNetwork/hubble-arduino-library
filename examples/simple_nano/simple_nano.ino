#include <HubbleNetwork.h>
#include <SimpleCLI.h>

SimpleCLI cli;

// Commands
Command provision;
Command send;

void provisionCallback(cmd* c) {
  Command cmd(c); // Create wrapper object

  // Get arguments
  Argument time_arg = cmd.getArg(0);
  Argument key_arg = cmd.getArg(1);

  // Convert
  int time_s = time_arg.getValue().toInt();
  const char* key_str = key_arg.getValue().c_str();

  HubbleNetwork.begin(time_s, key_str);
  HubbleNetwork.start(HubbleNetwork.ADV_INTERVAL_FAST);

  Serial.print("[SUCCESS] Device provisioned\n");
}

void sendCallback(cmd* c) {
  Command cmd(c);

  Argument arg = cmd.getArgument(0);
  String argVal = arg.getValue();

  size_t len = argVal.length();
  if (len > 13) {
    Serial.println("[ERROR] Message too long");
    return;
  }

  HubbleNetwork.setData(argVal.c_str());
}

void errorCallback(cmd_error* e) {
  CommandError cmdError(e); // Create wrapper object

  Serial.print("[ERROR] ");
  Serial.println(cmdError.toString());

  if (cmdError.hasCommand()) {
    Serial.print("Did you mean \"");
    Serial.print(cmdError.getCommand().toString());
    Serial.println("\"?");
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for USB serial (on boards like Leonardo, Micro, Nano 33, etc.)
  }

  cli.setOnError(errorCallback); // Set error Callback

  provision = cli.addBoundlessCommand("p/rovision", provisionCallback);
  send = cli.addSingleArgCmd("s/end", sendCallback);
}

void loop() {
  static String inputLine;

  HubbleNetwork.poll();

  // Read any available characters without blocking
  while (Serial.available() > 0) {
    char c = Serial.read();

    // Ignore carriage return (useful when using serial monitor with CR+LF)
    if (c == '\r') {
      continue;
    }

    // Newline means the user finished a command
    if (c == '\n') {
      // Only handle non-empty lines
      if (inputLine.length() > 0) {
        // Echo the user input
        Serial.print("# ");
        Serial.println(inputLine);

        // Parse the user input into the CLI
        cli.parse(inputLine);

        // Clear buffer for next command
        inputLine = "";
      }
    } else {
      // Accumulate characters into the current line
      inputLine += c;
    }
  }
}

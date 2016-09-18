# Communication

The ForumService communicates with external services like the web server in a RPC style fashion.

Each request will provide parameters in the form of a string array and an output stream. 

The request handler will write JSON to the output stream.

Example handler

   void Commands::version(const std::vector<std::string>& parameters, std::ostream& output)
   {
       Json::JsonWriter writer(output);
       writer
           << Json::objStart
               << Json::propertySafeName("version", VERSION)
           << Json::objEnd;
   }

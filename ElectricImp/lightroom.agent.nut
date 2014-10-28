params <-
{
    connected = false,
    mode = null,
    pixel_brightness = null,
    strobe_delay = null,
    rainbow_delay = null,
    co_spread_delay = null,
    color_change_cutoff = null,
    min_db = null,
    max_db = null,
    bass_freq = null,
    mid_freq = null,
    treble_freq = null
};

saved_response <- null;

// see https://electricimp.com/docs/resources/interactive/

function http_request_handler(request, response)
{
    try{
        local path = request.path.tolower();
        server.log("path: " + path);
        if(path == "/mode") 
        {
                local modeVal = request.query.value.tointeger();
                device.send("mode", modeVal);
                response.send(200, "OK");
        }
        else if(path == "/brightness") 
        {
                local val = request.query.value.tointeger();
                device.send("brightness", val);
                response.send(200, "OK");
        }
        else if (path == "/api/yo")
        {
            device.send("mode", 6);
        }
        else if (path == "/api/getall")
        {
            if(device.isconnected)
            {
                params.connected = true;    
            }
            else
            {
                params.connected = false;
            }
            response.header("Content-Type", "application/json");
            response.send(200, http.jsonencode(params));
        }
        else if (path == "/api/rgb")
        {
            local color = {
                red = 0,
                green = 0,
                blue = 0
            }
            
            if ("red" in request.query) {
                server.log("red: " + request.query["red"]);
                color.red = request.query["red"].tointeger();
            }
            if ("green" in request.query) {
                server.log("green: " + request.query["green"]);
                color.green = request.query["green"].tointeger();
            }
            if ("blue" in request.query) {
                server.log("blue: " + request.query["blue"]);
                color.blue = request.query["blue"].tointeger();
            }
            
            device.send("rgb", color);
            response.send(200, "OK");
            
        }
        else if (path.find("/api") == 0)
        {
            local parameter = split(path, "/")[1];
            
            if (request.method == "GET")
            {
                saved_response = response;
                device.send("getParam", parameter);
            }
            else if (request.method == "POST")
            {
                if ("value" in request.query)
                {
                    local data = {param = parameter, value = request.query.value.tointeger()};
                    saved_response = response;
                    device.send("setParam", data);
                }
                else
                {
                    response.send("400", "Value query param required");
                }
            }
        }
    } catch (ex) {
        response.send(500, "Error: " + ex);
    }
}

function initCallback(data)
{
    params = data;
}

function getParamCallback(data)
{
    if (data != null)
    {
        saved_response.send(200, data);
    }
    else 
    {
        saved_response.send(500, "Invalid parameter");
    }
}

function setParamCallback(data)
{
    if (data["success"])
    {
        saved_response.send(200, "OK");
        params[data.param] = data.value;
    }
    else
    {
        saved_response.send(500, "ERROR");
    }
}

device.onconnect(function(){device.send("init", params);});

http.onrequest(http_request_handler);

device.on("initCallback", initCallback);
device.on("getParamCallback", getParamCallback);
device.on("setParamCallback", setParamCallback)
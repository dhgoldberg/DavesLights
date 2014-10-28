server.log("Device Started");

function init(parameters)
{
    foreach(param, val in parameters)
    {
        parameters[param] = getTeensyParameter(param);
    }
    
    agent.send("initCallback", parameters);
}

function setMode(mode)
{
    server.log("Setting mode to: " + mode);
    teensy.write("SET MODE " + mode + ";");
    teensy.flush();
    imp.sleep(0.25);
    local ret = teensy.readstring();
    server.log("got back " + ret);
}

function setBrightness(val)
{
    server.log("Setting brightness to: " + val);
    teensy.write("SET PIXEL_BRIGHTNESS " + val + ";");
    teensy.flush();
}

function setRGB(color)
{
    teensy.write("SET FIXED_RED " + color.red + ";");
    teensy.flush();
    imp.sleep(0.05);
    teensy.write("SET FIXED_GREEN " + color.green + ";");
    teensy.flush();
    imp.sleep(0.05);
    teensy.write("SET FIXED_BLUE " + color.blue + ";");
    teensy.flush();
    imp.sleep(0.25);
    teensy.readstring();
}

function setTeensyParameter(data)
{
    server.log("setting " + data["param"] + " to " + data["value"]);
    teensy.write("SET " + data["param"] + " " + data["value"] + ";");
    teensy.flush();
    imp.sleep(0.25);
    local ret = teensy.readstring();
    server.log("got back " + ret);
    if (ret == "ERROR\r\n")
    {
        data["success"] <- false;
    }
    else if (ret.tointeger() != data["value"])
    {
        data["success"] <- false;
    }
    else
    {
        data["success"] <- true;
    }
    
    agent.send("setParamCallback", data);
}

function getTeensyParameter(param)
{
    server.log("Reqesting " + param + " from teensy");
    teensy.write("GET " + param + ";");
    teensy.flush();
    imp.sleep(0.25);
    local val = teensy.readstring();
    server.log("Got back " + val);
    //if (val == "")
    //{
    //    getTeensyParameter(param);
    //}
    //else if (val == "ERROR")
    if ((val == "ERROR\r\n") || (val == ""))
    {
        //agent.send("getParamCallback", null);
        return null;
    }
    else
    {
        //local ret = {};
        //ret[param] = val.tointeger();
        //agent.send("getParamCallback", {param= val.tointeger()});
        return val.tointeger();
    }
}

function getSingleParameter(param)
{
    agent.send("getParamCallback", getTeensyParameter(param));
}

teensy <- hardware.uart12;
teensy.configure(38400, 8, PARITY_NONE, 1, NO_CTSRTS);

agent.on("mode", setMode);
agent.on("brightness", setBrightness);
agent.on("rgb", setRGB);
agent.on("init", init);
agent.on("setParam", setTeensyParameter);
agent.on("getParam", getSingleParameter);
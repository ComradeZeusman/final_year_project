const http = require("http");
const mongoose = require("mongoose");

const DataSchema = new mongoose.Schema({
  timestamp: { type: Date, default: Date.now },
  deviceId: String,
  ownerName: String,
  password: String,
  firstBoot: { type: Boolean, default: true },
  isRegistered: { type: Boolean, default: false },
  location: String,
  bootCount: { type: Number, default: 0 },
  faceData: {
    faceId: Number,
    samples: [
      {
        sampleId: Number,
        timestamp: Date,
        data: Buffer,
      },
    ],
    enrolled: { type: Boolean, default: false },
    enrollmentComplete: { type: Boolean, default: false },
  },
});

const Data = mongoose.model("Data", DataSchema);

const connectWithRetry = async (retries = 5) => {
  for (let i = 0; i < retries; i++) {
    try {
      await mongoose.connect("mongodb://127.0.0.1:27017/myDatabase", {
        serverSelectionTimeoutMS: 5000,
        connectTimeoutMS: 10000,
        socketTimeoutMS: 45000,
      });
      console.log("MongoDB connected successfully");
      return true;
    } catch (err) {
      console.error(`Connection attempt ${i + 1} failed:`, err.message);
      if (i < retries - 1)
        await new Promise((resolve) => setTimeout(resolve, 2000));
    }
  }
  return false;
};

const handlers = {
  async status(req, res) {
    try {
      const device = await Data.findOne({}).sort({ timestamp: -1 });
      res.writeHead(200, { "Content-Type": "application/json" });
      res.end(
        JSON.stringify({
          firstBoot: device ? device.firstBoot : true,
          isRegistered: device ? device.isRegistered : false,
          faceEnrolled: device?.faceData?.enrollmentComplete || false,
        })
      );
    } catch (err) {
      res.writeHead(500);
      res.end(JSON.stringify({ error: err.message }));
    }
  },

  async register(req, body) {
    try {
      const data = JSON.parse(body);
      const newDevice = new Data({
        deviceId: data.deviceId,
        ownerName: data.ownerName,
        password: data.password,
        location: data.location,
        firstBoot: false,
        isRegistered: true,
        faceData: {
          faceId: 1,
          enrolled: false,
          samples: [],
        },
      });
      return await newDevice.save();
    } catch (err) {
      throw new Error(`Registration failed: ${err.message}`);
    }
  },

  async bootCount(req, res) {
    try {
      const device = await Data.findOne({}).sort({ timestamp: -1 });
      res.writeHead(200, { "Content-Type": "application/json" });
      res.end(
        JSON.stringify({
          bootCount: device ? device.bootCount : 0,
        })
      );
    } catch (err) {
      res.writeHead(500);
      res.end(JSON.stringify({ error: err.message }));
    }
  },

  async data(req, body) {
    try {
      const data = JSON.parse(body);
      const device = await Data.findOne({ deviceId: data.deviceId });

      if (!device) {
        const newDevice = new Data({
          deviceId: data.deviceId,
          firstBoot: true,
          bootCount: 1,
        });
        return await newDevice.save();
      }

      device.bootCount += 1;
      device.firstBoot = false;
      return await device.save();
    } catch (err) {
      throw new Error(`Data update failed: ${err.message}`);
    }
  },

  async faceSample(req, body) {
    const data = JSON.parse(body);
    const device = await Data.findOne({ deviceId: data.deviceId });
    if (!device) throw new Error("Device not found");

    device.faceData.samples.push({
      sampleId: data.sampleId,
      timestamp: new Date(),
      data: Buffer.from(data.faceData, "base64"),
    });

    if (device.faceData.samples.length >= 5) {
      device.faceData.enrolled = true;
      device.faceData.enrollmentComplete = true;
    }

    await device.save();
    return {
      samplesCollected: device.faceData.samples.length,
      enrollmentComplete: device.faceData.enrollmentComplete,
    };
  },

  async verify(req, body) {
    const data = JSON.parse(body);
    const device = await Data.findOne({ deviceId: data.deviceId });
    if (!device || !device.faceData.enrollmentComplete) {
      throw new Error("Device not found or enrollment incomplete");
    }
    return { verified: true };
  },
};

// Add MongoDB status endpoint
handlers.status = async (req, res) => {
  res.writeHead(200, { "Content-Type": "application/json" });
  res.end(
    JSON.stringify({
      mongoConnected: mongoose.connection.readyState === 1,
    })
  );
};

const server = http.createServer(async (req, res) => {
  res.setHeader("Access-Control-Allow-Origin", "*");
  res.setHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  res.setHeader("Access-Control-Allow-Headers", "Content-Type");

  if (req.method === "OPTIONS") {
    res.writeHead(200);
    res.end();
    return;
  }

  const getBody = async (req) => {
    let body = "";
    for await (const chunk of req) {
      body += chunk;
    }
    return body;
  };

  try {
    if (req.method === "GET") {
      switch (req.url) {
        case "/status":
          await handlers.status(req, res);
          return;
        case "/bootCount":
          await handlers.bootCount(req, res);
          return;
      }
    }

    const body = await getBody(req);
    let result;

    if (req.method === "POST") {
      switch (req.url) {
        case "/register":
          result = await handlers.register(req, body);
          break;
        case "/data":
          result = await handlers.data(req, body);
          break;
        case "/face-sample":
          result = await handlers.faceSample(req, body);
          break;
        case "/verify":
          result = await handlers.verify(req, body);
          break;
        default:
          throw new Error("Not Found");
      }

      res.writeHead(200, { "Content-Type": "application/json" });
      res.end(JSON.stringify(result));
    }
  } catch (err) {
    const statusCode = err.message === "Not Found" ? 404 : 500;
    res.writeHead(statusCode);
    res.end(JSON.stringify({ error: err.message }));
  }
});

const startServer = async () => {
  if (await connectWithRetry()) {
    server.listen(3000, "0.0.0.0", () => {
      console.log("Server running on port 3000");
    });
  } else {
    console.error("Failed to connect to MongoDB after retries");
    process.exit(1);
  }
};

startServer();

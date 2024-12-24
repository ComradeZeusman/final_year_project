// server.js
const http = require("http");
const { MongoClient } = require("mongodb");

const url = "mongodb://127.0.0.1:27017";
const dbName = "esp32_camera";
let db;

async function connectDB() {
  try {
    const client = await MongoClient.connect(url);
    db = client.db(dbName);
    console.log("Connected to MongoDB");
  } catch (err) {
    console.error("MongoDB connection error:", err);
  }
}

connectDB();

const server = http.createServer(async (req, res) => {
  if (req.method === "POST" && req.url === "/upload") {
    let imageData = [];

    req.on("data", (chunk) => {
      imageData.push(chunk);
    });

    req.on("end", async () => {
      try {
        const imageBuffer = Buffer.concat(imageData);
        const collection = db.collection("images");

        await collection.insertOne({
          data: imageBuffer,
          timestamp: new Date(),
          contentType: req.headers["content-type"] || "image/jpeg",
        });

        res.writeHead(200, {
          "Content-Type": "text/plain",
          "Access-Control-Allow-Origin": "*",
        });
        res.end("Image uploaded successfully");
      } catch (err) {
        console.error("Upload error:", err);
        res.writeHead(500, { "Content-Type": "text/plain" });
        res.end("Error saving image");
      }
    });
  } else {
    res.writeHead(404, { "Content-Type": "text/plain" });
    res.end("Not found");
  }
});

const PORT = 3000;
server.listen(PORT, () => {
  console.log(`Server running on port ${PORT}`);
});

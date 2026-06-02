const express = require('express');
const cors = require('cors');

const app = express();
app.use(express.json());
app.use(cors());

// The local IP of your ESP32
const ESP32_IP = 'http://192.168.4.1';

// --- THE AI LOGIC ENGINE ---
app.post('/chat', async (req, res) => {
    const userMessage = req.body.message.toLowerCase();
    let hardwareCommand = '';

    console.log(`\n🗣️  You said: "${userMessage}"`);

    // 1. Parse Intent 
    if (userMessage.includes('sleep') || userMessage.includes('tired') || userMessage.includes('night')) {
        hardwareCommand = 'SLEEP 4000';
        console.log(`🧠 AIBI thinks: "I should go to sleep."`);
    } 
    else if (userMessage.includes('happy') || userMessage.includes('awesome') || userMessage.includes('good boy')) {
        hardwareCommand = 'HAPPY 3000';
        console.log(`🧠 AIBI thinks: "I am being praised!"`);
    } 
    else if (userMessage.includes('wake') || userMessage.includes('morning')) {
        hardwareCommand = 'WAKE 2000';
        console.log(`🧠 AIBI thinks: "Time to wake up."`);
    } 
    else {
        // Default behavior
        const calcDuration = Math.min(8000, Math.max(1500, userMessage.length * 80));
        hardwareCommand = `TALK ${calcDuration}`;
        console.log(`🧠 AIBI thinks: "I need to respond to this..."`);
    }

    // 2. The Hardware Ping
    try {
        console.log(`⚡ Sending physical command -> ${hardwareCommand}`);
        const espResponse = await fetch(`${ESP32_IP}/cmd?c=${encodeURIComponent(hardwareCommand)}`);
        
        if (espResponse.ok) {
            res.json({ success: true, action: hardwareCommand });
        } else {
            res.status(500).json({ error: "ESP32 rejected the command" });
        }
    } catch (error) {
        console.error("❌ FATAL: Could not reach ESP32. Is your laptop on AIBI's Wi-Fi?");
        res.status(500).json({ error: "Hardware unreachable" });
    }
});

app.listen(3000, () => {
    console.log('====================================');
    console.log('🧠 AIBI NEURAL NETWORK IS ONLINE 🧠');
    console.log('Listening for text on http://localhost:3000/chat');
    console.log('====================================');
});
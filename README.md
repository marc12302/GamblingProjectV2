![madafaca](https://github.com/user-attachments/assets/7ea5b267-63c8-42e4-a5d5-79a3e2129a9b)
# 🌊 Poseidon Casino - Modern Online Gaming Platform

<div align="center">
  <img src="https://img.shields.io/badge/C++-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" />
  <img src="https://img.shields.io/badge/SQLite-07405E?style=for-the-badge&logo=sqlite&logoColor=white" />
  <img src="https://img.shields.io/badge/HTML5-E34F26?style=for-the-badge&logo=html5&logoColor=white" />
  <img src="https://img.shields.io/badge/CSS3-1572B6?style=for-the-badge&logo=css3&logoColor=white" />
  <img src="https://img.shields.io/badge/JavaScript-F7DF1E?style=for-the-badge&logo=javascript&logoColor=black" />
  <img src="https://img.shields.io/badge/JWT-black?style=for-the-badge&logo=JSON%20web%20tokens" />
</div>

<div align="center">
  <h3>⚡ A high-performance online casino platform built with C++ backend and modern web technologies</h3>
  <p>
    <a href="#features">Features</a> •
    <a href="#tech-stack">Tech Stack</a> •
    <a href="#installation">Installation</a> •
    <a href="#usage">Usage</a> •
    <a href="#api-documentation">API</a> •
    <a href="#screenshots">Screenshots</a>
  </p>
</div>

---

## 🎯 Overview

Poseidon Casino is a cutting-edge online gambling platform that combines the raw performance of C++ with modern web technologies to deliver an exceptional gaming experience. Named after the Greek god of the seas, the platform features an immersive ocean-themed design with stunning visual effects and smooth animations.

## ✨ Features

### 🔐 **Secure Authentication System**
- JWT-based authentication with 2-hour token expiration
- Secure user registration and login
- Password hashing (ready for production)
- Persistent sessions with "Remember Me" functionality

### 🎮 **Interactive Games**
- **🎲 Poseidon's Dice** - Classic dice game with multipliers up to 35x
  - Predict the sum of two dice
  - Real-time animations
  - Detailed betting history
  
- **🚀 Rocket Crash** - Exciting multiplier game
  - Progressive multiplier system
  - Auto cash-out feature
  - Live bets from other players
  - Real-time crash animation

### 📊 **Comprehensive Dashboard**
- Real-time balance tracking
- Detailed gaming statistics
  - Total games played
  - Win rate percentage
  - Biggest win tracking
  - Session history
- Quick fund management
- Responsive design for all devices

### 🤖 **Intelligent House Edge System**
- Advanced supervisor algorithm
- Dynamic probability adjustment
- Player retention optimization
- Maintains 5% house edge
- Fairness with profitability balance

### 🎨 **Modern UI/UX**
- Ocean-themed design with Poseidon aesthetics
- Neon blue color scheme (#00D4FF, #0066FF)
- Smooth animations and transitions
- Dark mode for comfortable gaming
- Fully responsive layout

## 🛠️ Tech Stack

### Backend
- **C++20** - Core backend language
- **Crow Framework** - Fast HTTP server
- **SQLite3** - Lightweight database
- **JWT-CPP** - Token authentication
- **OpenSSL** - Cryptography

### Frontend
- **HTML5** - Semantic markup
- **CSS3** - Modern styling with animations
- **JavaScript ES6+** - Interactive functionality
- **Vanilla JS** - No framework dependencies

### Architecture
- **RESTful API** design
- **MVC** pattern implementation
- **Modular** component structure
- **Responsive** mobile-first approach

## 📦 Installation

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install g++ cmake libsqlite3-dev libssl-dev

# macOS
brew install gcc cmake sqlite openssl
```

### Clone Repository
```bash
git clone https://github.com/yourusername/poseidon-casino.git
cd poseidon-casino
```

### Build Backend
```bash
# Create build directory
mkdir build && cd build

# Compile
g++ -std=c++20 ../src/main.cpp -lcrow -lsqlite3 -lssl -lcrypto -pthread -o casino_server

# Or using CMake
cmake ..
make
```

## 🚀 Usage

### Start the Server
```bash
./casino_server
# Server starts on http://localhost:18080
```

### Access the Platform
1. Open your browser and navigate to `http://localhost:18080`
2. Create a new account or login
3. Add funds to your account (demo mode)
4. Start playing!

## 📡 API Documentation

### Authentication Endpoints

#### Register
```http
POST /api/register
Content-Type: application/json

{
  "username": "player123",
  "password": "securepassword",
  "email": "player@example.com"
}
```

#### Login
```http
POST /api/login
Content-Type: application/json

{
  "username": "player123",
  "password": "securepassword"
}
```

### Game Endpoints

#### Get Balance
```http
GET /api/balance
Authorization: Bearer <token>
```

#### Play Dice
```http
POST /api/games/dice
Authorization: Bearer <token>
Content-Type: application/json

{
  "bet": 50,
  "target": 7,
  "username": "player123"
}
```

#### Play Rocket
```http
POST /api/games/rocket
Authorization: Bearer <token>
Content-Type: application/json

{
  "action": "start",
  "bet": 100,
  "username": "player123"
}
```

## 🎮 Game Rules

### 🎲 Poseidon's Dice
- Choose a target sum (2-12)
- Place your bet
- Roll two dice
- Win if the sum matches your prediction
- Payouts vary by probability (2 and 12 pay 35:1)

### 🚀 Rocket Crash
- Place your bet before launch
- Watch the multiplier increase
- Cash out before the rocket crashes
- Auto cash-out option available
- Higher risk = higher reward

## 🔒 Security Features

- **JWT Authentication** - Secure token-based auth
- **CORS Protection** - API security headers
- **Input Validation** - Client and server-side
- **SQL Injection Prevention** - Prepared statements
- **Rate Limiting** - (Implement in production)
- **HTTPS Support** - (Configure for production)

## 📈 Performance

- **High-Performance Backend** - C++ for maximum speed
- **Optimized Database Queries** - Indexed tables
- **Efficient WebSocket** - Real-time updates
- **Lazy Loading** - Resources loaded on demand
- **Minified Assets** - Reduced payload size

## 🎨 Customization

### Themes
Modify color scheme in `common.css`:
```css
:root {
    --primary: #00D4FF;
    --secondary: #0066FF;
    --dark: #0A0E1B;
    --success: #00FF88;
    --error: #FF4444;
}
```

### Game Settings
Adjust house edge in `main.cpp`:
```cpp
struct SupervisorSystem {
    double house_edge = 0.05; // 5% default
    double player_retention_factor = 0.85;
}
```


## 📝 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Crow Framework for the excellent C++ web framework
- SQLite for the reliable database engine
- The open-source community for inspiration


---

<div align="center">
  <p>Made with ❤️ and ☕ by Marc</p>
  <p>⭐ Star this repository if you find it helpful!</p>
</div>

#include "deps/crow_all.h"
#include <sqlite3.h>
#include <jwt-cpp/jwt.h>
#include <random>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <fstream>
#include <sstream>

using namespace std;

class CasinoBackend {
private:
    sqlite3* db;
    string jwt_secret = "poseidon_secret_key_2024";
    mt19937 rng;
    mutex db_mutex;

    // Helper pentru a adăuga CORS headers la response
    void addCorsHeaders(crow::response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    }

    // Sistem supervisor pentru profit
    struct SupervisorSystem {
        double house_edge = 0.05; // 5% avantaj pentru cazinou
        double player_retention_factor = 0.85; // Factor pentru dependență
        double current_session_profit = 0.0;
        int consecutive_wins = 0;
        int consecutive_losses = 0;

        double adjustProbability(double base_prob, double bet_amount) {
            // Ajustăm probabilitatea în funcție de istoricul jucătorului
            if (consecutive_losses > 3) {
                // Creștem șansele de câștig pentru a menține dependența
                return min(base_prob + 0.1, 0.7);
            } else if (consecutive_wins > 2) {
                // Scădem șansele pentru a asigura profitul cazinoului
                return max(base_prob - 0.15, 0.2);
            }
            return base_prob;
        }
    };

    unordered_map<string, SupervisorSystem> player_supervisors;

public:
    CasinoBackend() : rng(chrono::steady_clock::now().time_since_epoch().count()) {
        initDatabase();
    }

    ~CasinoBackend() {
        if (db) sqlite3_close(db);
    }

    void initDatabase() {
        sqlite3_open("poseidon_casino.db", &db);

        const char* users_table = R"(
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT UNIQUE NOT NULL,
                password TEXT NOT NULL,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP
            );
        )";

        const char* balances_table = R"(
            CREATE TABLE IF NOT EXISTS balances (
                user_id INTEGER PRIMARY KEY,
                balance REAL DEFAULT 1000.0,
                last_updated DATETIME DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (user_id) REFERENCES users (id)
            );
        )";

        const char* transactions_table = R"(
            CREATE TABLE IF NOT EXISTS transactions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                user_id INTEGER,
                amount REAL,
                type TEXT,
                game TEXT,
                timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
                FOREIGN KEY (user_id) REFERENCES users (id)
            );
        )";

        sqlite3_exec(db, users_table, 0, 0, 0);
        sqlite3_exec(db, balances_table, 0, 0, 0);
        sqlite3_exec(db, transactions_table, 0, 0, 0);
    }

    string generateJWT(int user_id, const string& username) {
        auto now = chrono::system_clock::now();
        auto exp = now + chrono::hours(2); // Token expiră în 2 ore

        auto token = jwt::create()
            .set_issuer("poseidon_casino")
            .set_type("JWT")
            .set_issued_at(now)
            .set_expires_at(exp)
            .set_payload_claim("user_id", jwt::claim(to_string(user_id)))
            .set_payload_claim("username", jwt::claim(username))
            .sign(jwt::algorithm::hs256{jwt_secret});

        return token;
    }

    bool verifyJWT(const string& token) {
        try {
            auto decoded = jwt::decode(token);
            auto verifier = jwt::verify()
                .allow_algorithm(jwt::algorithm::hs256{jwt_secret})
                .with_issuer("poseidon_casino");
            verifier.verify(decoded);
            return true;
        } catch (...) {
            return false;
        }
    }

    int getUserIdFromToken(const string& token) {
        try {
            auto decoded = jwt::decode(token);
            return stoi(decoded.get_payload_claim("user_id").as_string());
        } catch (...) {
            return -1;
        }
    }

    void setupRoutes(crow::SimpleApp& app) {
        // Servește fișierele statice
        app.route_dynamic("/")([](const crow::request& req, crow::response& res) {
            ifstream file("static/index.html");
            stringstream buffer;
            buffer << file.rdbuf();
            res.set_header("Content-Type", "text/html");
            res.write(buffer.str());
            res.end();
        });

        app.route_dynamic("/login")([](const crow::request& req, crow::response& res) {
            ifstream file("static/login.html");
            stringstream buffer;
            buffer << file.rdbuf();
            res.set_header("Content-Type", "text/html");
            res.write(buffer.str());
            res.end();
        });

        app.route_dynamic("/register")([](const crow::request& req, crow::response& res) {
            ifstream file("static/register.html");
            stringstream buffer;
            buffer << file.rdbuf();
            res.set_header("Content-Type", "text/html");
            res.set_header("Cache-Control", "no-cache, no-store, must-revalidate");
            res.set_header("Pragma", "no-cache");
            res.set_header("Expires", "0");
            res.write(buffer.str());
            res.end();
        });

        app.route_dynamic("/dashboard")([this](const crow::request& req, crow::response& res) {
            // Pentru dashboard, doar servim fișierul HTML
            // Verificarea token-ului se face în frontend prin JavaScript
            ifstream file("static/dashboard.html");
            stringstream buffer;
            buffer << file.rdbuf();
            res.set_header("Content-Type", "text/html");
            res.write(buffer.str());
            res.end();
        });

        app.route_dynamic("/games/dice")([](const crow::request& req, crow::response& res) {
            // Servim direct fișierul HTML
            ifstream file("static/dice.html");
            stringstream buffer;
            buffer << file.rdbuf();
            res.set_header("Content-Type", "text/html");
            res.write(buffer.str());
            res.end();
        });

        app.route_dynamic("/games/rocket")([](const crow::request& req, crow::response& res) {
            // Servim direct fișierul HTML
            ifstream file("static/rocket.html");
            stringstream buffer;
            buffer << file.rdbuf();
            res.set_header("Content-Type", "text/html");
            res.write(buffer.str());
            res.end();
        });

        // API Routes
        app.route_dynamic("/api/register").methods("POST"_method)([this](const crow::request& req) {
            auto x = crow::json::load(req.body);
            if (!x) {
                crow::response res(400, "Invalid JSON");
                res.set_header("Access-Control-Allow-Origin", "*");
                return res;
            }

            string username = x["username"].s();
            string password = x["password"].s(); // În producție, hash-uiește parola!

            lock_guard<mutex> lock(db_mutex);

            // Verifică dacă utilizatorul există
            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(db, "SELECT id FROM users WHERE username = ?", -1, &stmt, 0);
            sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                sqlite3_finalize(stmt);
                crow::response res(409, R"({"error": "Username already exists"})");
                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_header("Content-Type", "application/json");
                return res;
            }
            sqlite3_finalize(stmt);

            // Inserează utilizatorul nou
            sqlite3_prepare_v2(db, "INSERT INTO users (username, password) VALUES (?, ?)", -1, &stmt, 0);
            sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            int user_id = sqlite3_last_insert_rowid(db);

            // Creează balanța inițială
            sqlite3_prepare_v2(db, "INSERT INTO balances (user_id, balance) VALUES (?, 1000.0)", -1, &stmt, 0);
            sqlite3_bind_int(stmt, 1, user_id);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            string token = generateJWT(user_id, username);

            crow::json::wvalue response;
            response["token"] = token;
            response["username"] = username;
            response["balance"] = 1000.0;

            crow::response res(200, response);
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Content-Type", "application/json");
            return res;
        });

        app.route_dynamic("/api/login").methods("POST"_method)([this](const crow::request& req) {
            auto x = crow::json::load(req.body);
            if (!x) {
                crow::response res(400, "Invalid JSON");
                res.set_header("Access-Control-Allow-Origin", "*");
                return res;
            }

            string username = x["username"].s();
            string password = x["password"].s();

            lock_guard<mutex> lock(db_mutex);

            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(db, "SELECT id, password FROM users WHERE username = ?", -1, &stmt, 0);
            sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                int user_id = sqlite3_column_int(stmt, 0);
                string stored_password = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                sqlite3_finalize(stmt);

                if (password == stored_password) {
                    string token = generateJWT(user_id, username);

                    // Obține balanța
                    sqlite3_prepare_v2(db, "SELECT balance FROM balances WHERE user_id = ?", -1, &stmt, 0);
                    sqlite3_bind_int(stmt, 1, user_id);
                    sqlite3_step(stmt);
                    double balance = sqlite3_column_double(stmt, 0);
                    sqlite3_finalize(stmt);

                    crow::json::wvalue response;
                    response["token"] = token;
                    response["username"] = username;
                    response["balance"] = balance;

                    crow::response res(200, response);
                    res.set_header("Access-Control-Allow-Origin", "*");
                    res.set_header("Content-Type", "application/json");
                    return res;
                }
            } else {
                sqlite3_finalize(stmt);
            }

            crow::response res(401, R"({"error": "Invalid credentials"})");
            res.set_header("Access-Control-Allow-Origin", "*");
            res.set_header("Content-Type", "application/json");
            return res;
        });

        app.route_dynamic("/api/balance").methods("GET"_method)([this](const crow::request& req) {
            auto auth = req.get_header_value("Authorization");
            if (auth.empty()) {
                crow::response res(401, "No token provided");
                res.set_header("Access-Control-Allow-Origin", "*");
                return res;
            }

            string token = auth.substr(7); // Remove "Bearer "
            if (!verifyJWT(token)) {
                crow::response res(401, "Invalid token");
                res.set_header("Access-Control-Allow-Origin", "*");
                return res;
            }

            int user_id = getUserIdFromToken(token);

            lock_guard<mutex> lock(db_mutex);
            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(db, "SELECT balance FROM balances WHERE user_id = ?", -1, &stmt, 0);
            sqlite3_bind_int(stmt, 1, user_id);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                double balance = sqlite3_column_double(stmt, 0);
                sqlite3_finalize(stmt);

                crow::json::wvalue response;
                response["balance"] = balance;

                crow::response res(200, response);
                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_header("Content-Type", "application/json");
                return res;
            }

            sqlite3_finalize(stmt);
            crow::response res(404, "Balance not found");
            res.set_header("Access-Control-Allow-Origin", "*");
            return res;
        });

        app.route_dynamic("/api/add-funds").methods("POST"_method)([this](const crow::request& req) {
            auto auth = req.get_header_value("Authorization");
            if (auth.empty()) return crow::response(401, "No token provided");

            string token = auth.substr(7);
            if (!verifyJWT(token)) return crow::response(401, "Invalid token");

            auto x = crow::json::load(req.body);
            if (!x) return crow::response(400, "Invalid JSON");

            double amount = x["amount"].d();
            if (amount <= 0) return crow::response(400, "Invalid amount");

            int user_id = getUserIdFromToken(token);

            lock_guard<mutex> lock(db_mutex);

            // Actualizează balanța
            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(db,
                "UPDATE balances SET balance = balance + ?, last_updated = CURRENT_TIMESTAMP WHERE user_id = ?",
                -1, &stmt, 0);
            sqlite3_bind_double(stmt, 1, amount);
            sqlite3_bind_int(stmt, 2, user_id);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            // Înregistrează tranzacția
            sqlite3_prepare_v2(db,
                "INSERT INTO transactions (user_id, amount, type, game) VALUES (?, ?, 'deposit', 'none')",
                -1, &stmt, 0);
            sqlite3_bind_int(stmt, 1, user_id);
            sqlite3_bind_double(stmt, 2, amount);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            // Obține noua balanță
            sqlite3_prepare_v2(db, "SELECT balance FROM balances WHERE user_id = ?", -1, &stmt, 0);
            sqlite3_bind_int(stmt, 1, user_id);
            sqlite3_step(stmt);
            double new_balance = sqlite3_column_double(stmt, 0);
            sqlite3_finalize(stmt);

            crow::json::wvalue response;
            response["balance"] = new_balance;
            response["message"] = "Funds added successfully";

            return crow::response(200, response);
        });

        // Jocul cu zaruri
        app.route_dynamic("/api/games/dice").methods("POST"_method)([this](const crow::request& req) {
            auto auth = req.get_header_value("Authorization");
            if (auth.empty()) return crow::response(401, "No token provided");

            string token = auth.substr(7);
            if (!verifyJWT(token)) return crow::response(401, "Invalid token");

            auto x = crow::json::load(req.body);
            if (!x) return crow::response(400, "Invalid JSON");

            double bet = x["bet"].d();
            int target = x["target"].i(); // Suma țintă pentru zaruri

            int user_id = getUserIdFromToken(token);
            auto username = x["username"].s();

            lock_guard<mutex> lock(db_mutex);

            // Verifică balanța
            sqlite3_stmt* stmt;
            sqlite3_prepare_v2(db, "SELECT balance FROM balances WHERE user_id = ?", -1, &stmt, 0);
            sqlite3_bind_int(stmt, 1, user_id);
            sqlite3_step(stmt);
            double balance = sqlite3_column_double(stmt, 0);
            sqlite3_finalize(stmt);

            if (balance < bet) {
                return crow::response(400, R"({"error": "Insufficient balance"})");
            }

            // Sistemul supervisor
            if (player_supervisors.find(username) == player_supervisors.end()) {
                player_supervisors[username] = SupervisorSystem();
            }
            auto& supervisor = player_supervisors[username];

            // Generează rezultatul zarurilor
            uniform_int_distribution<> dis(1, 6);
            int dice1 = dis(rng);
            int dice2 = dis(rng);
            int sum = dice1 + dice2;

            // Calculează probabilitatea de bază și ajustează cu supervisorul
            double base_probability = 1.0 / 11.0 * (12 - abs(7 - target));
            double adjusted_probability = supervisor.adjustProbability(base_probability, bet);

            // Decide rezultatul cu probabilitatea ajustată
            uniform_real_distribution<> prob_dis(0.0, 1.0);
            bool should_win = prob_dis(rng) < adjusted_probability;

            // Forțează rezultatul dacă e necesar pentru supervisor
            if (should_win && sum != target) {
                // Ajustăm zarurile să dea suma dorită
                if (target >= 2 && target <= 12) {
                    dice1 = min(target - 1, 6);
                    dice2 = target - dice1;
                }
            } else if (!should_win && sum == target) {
                // Schimbăm unul dintre zaruri
                dice1 = (dice1 % 6) + 1;
                sum = dice1 + dice2;
            }

            bool won = (sum == target);
            double payout = won ? bet * 2.5 : -bet; // Plată 2.5:1 pentru câștig

            // Actualizează supervisorul
            if (won) {
                supervisor.consecutive_wins++;
                supervisor.consecutive_losses = 0;
            } else {
                supervisor.consecutive_losses++;
                supervisor.consecutive_wins = 0;
            }
            supervisor.current_session_profit += (won ? -payout : bet);

            // Actualizează balanța
            double new_balance = balance + payout;
            sqlite3_prepare_v2(db,
                "UPDATE balances SET balance = ?, last_updated = CURRENT_TIMESTAMP WHERE user_id = ?",
                -1, &stmt, 0);
            sqlite3_bind_double(stmt, 1, new_balance);
            sqlite3_bind_int(stmt, 2, user_id);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            // Înregistrează tranzacția
            sqlite3_prepare_v2(db,
                "INSERT INTO transactions (user_id, amount, type, game) VALUES (?, ?, ?, 'dice')",
                -1, &stmt, 0);
            sqlite3_bind_int(stmt, 1, user_id);
            sqlite3_bind_double(stmt, 2, payout);
            sqlite3_bind_text(stmt, 3, won ? "win" : "loss", -1, SQLITE_STATIC);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);

            crow::json::wvalue response;
            response["dice1"] = dice1;
            response["dice2"] = dice2;
            response["sum"] = sum;
            response["won"] = won;
            response["payout"] = payout;
            response["balance"] = new_balance;

            return crow::response(200, response);
        });

        // Jocul cu racheta
        app.route_dynamic("/api/games/rocket").methods("POST"_method)([this](const crow::request& req) {
            auto auth = req.get_header_value("Authorization");
            if (auth.empty()) return crow::response(401, "No token provided");

            string token = auth.substr(7);
            if (!verifyJWT(token)) return crow::response(401, "Invalid token");

            auto x = crow::json::load(req.body);
            if (!x) return crow::response(400, "Invalid JSON");

            string action = x["action"].s();

            if (action == "start") {
                double bet = x["bet"].d();
                int user_id = getUserIdFromToken(token);
                auto username = x["username"].s();

                lock_guard<mutex> lock(db_mutex);

                // Verifică balanța
                sqlite3_stmt* stmt;
                sqlite3_prepare_v2(db, "SELECT balance FROM balances WHERE user_id = ?", -1, &stmt, 0);
                sqlite3_bind_int(stmt, 1, user_id);
                sqlite3_step(stmt);
                double balance = sqlite3_column_double(stmt, 0);
                sqlite3_finalize(stmt);

                if (balance < bet) {
                    return crow::response(400, R"({"error": "Insufficient balance"})");
                }

                // Sistemul supervisor
                if (player_supervisors.find(username) == player_supervisors.end()) {
                    player_supervisors[username] = SupervisorSystem();
                }
                auto& supervisor = player_supervisors[username];

                // Generează punctul de explozie cu distribuție exponențială
                exponential_distribution<> exp_dis(0.5);
                double raw_crash = 1.0 + exp_dis(rng);

                // Ajustează cu supervisorul
                if (supervisor.consecutive_losses > 3) {
                    raw_crash *= 1.5; // Crește șansele
                } else if (supervisor.consecutive_wins > 2) {
                    raw_crash *= 0.7; // Scade șansele
                }

                double crash_point = max(1.01, min(raw_crash, 10.0));

                // Scade balanța pentru pariu
                sqlite3_prepare_v2(db,
                    "UPDATE balances SET balance = balance - ? WHERE user_id = ?",
                    -1, &stmt, 0);
                sqlite3_bind_double(stmt, 1, bet);
                sqlite3_bind_int(stmt, 2, user_id);
                sqlite3_step(stmt);
                sqlite3_finalize(stmt);

                crow::json::wvalue response;
                response["crash_point"] = crash_point;
                response["bet"] = bet;
                response["game_id"] = to_string(user_id) + "_" + to_string(time(0));

                return crow::response(200, response);

            } else if (action == "cashout") {
                double multiplier = x["multiplier"].d();
                double bet = x["bet"].d();
                double crash_point = x["crash_point"].d();
                int user_id = getUserIdFromToken(token);
                auto username = x["username"].s();

                bool won = multiplier <= crash_point;
                double payout = won ? bet * multiplier : 0;

                lock_guard<mutex> lock(db_mutex);

                // Actualizează supervisorul
                auto& supervisor = player_supervisors[username];
                if (won) {
                    supervisor.consecutive_wins++;
                    supervisor.consecutive_losses = 0;
                } else {
                    supervisor.consecutive_losses++;
                    supervisor.consecutive_wins = 0;
                }

                if (won) {
                    // Adaugă câștigul
                    sqlite3_stmt* stmt;
                    sqlite3_prepare_v2(db,
                        "UPDATE balances SET balance = balance + ? WHERE user_id = ?",
                        -1, &stmt, 0);
                    sqlite3_bind_double(stmt, 1, payout);
                    sqlite3_bind_int(stmt, 2, user_id);
                    sqlite3_step(stmt);
                    sqlite3_finalize(stmt);

                    // Înregistrează tranzacția
                    sqlite3_prepare_v2(db,
                        "INSERT INTO transactions (user_id, amount, type, game) VALUES (?, ?, 'win', 'rocket')",
                        -1, &stmt, 0);
                    sqlite3_bind_int(stmt, 1, user_id);
                    sqlite3_bind_double(stmt, 2, payout - bet);
                    sqlite3_step(stmt);
                    sqlite3_finalize(stmt);
                }

                // Obține balanța actualizată
                sqlite3_stmt* stmt;
                sqlite3_prepare_v2(db, "SELECT balance FROM balances WHERE user_id = ?", -1, &stmt, 0);
                sqlite3_bind_int(stmt, 1, user_id);
                sqlite3_step(stmt);
                double new_balance = sqlite3_column_double(stmt, 0);
                sqlite3_finalize(stmt);

                crow::json::wvalue response;
                response["won"] = won;
                response["payout"] = payout;
                response["balance"] = new_balance;
                response["crashed_at"] = crash_point;

                return crow::response(200, response);
            }

            return crow::response(400, "Invalid action");
        });

        // Servește fișierele statice
        /*app.route_dynamic("/static/<path>")([](string path) {
            ifstream file("static/" + path);
            if (!file) {
                return crow::response(404);
            }

            stringstream buffer;
            buffer << file.rdbuf();

            crow::response res;
            res.body = buffer.str();

            // Setează Content-Type bazat pe extensie
            if (path.ends_with(".css")) {
                res.set_header("Content-Type", "text/css");
            } else if (path.ends_with(".js")) {
                res.set_header("Content-Type", "application/javascript");
            } else if (path.ends_with(".html")) {
                res.set_header("Content-Type", "text/html");
            }

            return res;
        });*/
    }
};

int main() {
    crow::SimpleApp app;
    CasinoBackend backend;

    // Configurare CORS - IMPORTANT: trebuie înainte de setupRoutes
    app.route_dynamic("/api/<path>").methods("OPTIONS"_method)([](const crow::request& req, string path_param) {
        crow::response res;
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        res.code = 204;
        return res;
    });

    backend.setupRoutes(app);

    cout << "Poseidon Casino Backend pornit pe portul 18080..." << endl;
    app.port(18080).multithreaded().run();

    return 0;
}
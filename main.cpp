#include<bits/stdc++.h>
#include<fstream>
#include<sstream>

using namespace std;

// Forward declarations for classes
class MarketDataLoader;
class Portfolio;
class Stock;
class Order;
class MarketOrder;
class LimitOrder;
class TradingStrategy;
class MovingAverageStrategy;
class RSIStrategy;
class MeanReversionStrategy;
class TradeEngine;

enum class OrderType { MARKET, LIMIT };


// function to stroe transactions
void logTransaction(const string& orderType, const string& symbol, int quantity, double price, const string& transactionType) {
    ofstream logFile("log.txt", ios::app);  // Open the log file in append mode
    if (!logFile.is_open()) {
        cout << "Error: Could not open transaction log file." << endl;
        return;
    }

    double totalValue = quantity * price;

    // Log the transaction in CSV format: TransactionType, OrderType, Symbol, Quantity, Price, TotalValue
    logFile << transactionType << ", "
            << orderType << ", "
            << symbol << ", "
            << quantity << ", "
            << price << ", "
            << totalValue << endl;

    logFile.close();  // Close the log file after writing

}
class MarketDataLoader {
public:
    struct MarketData {
        string date;
        double openPrice, highPrice, lowPrice, closePrice, volume;
        double gain, loss, avgGain, avgLoss, rsi, movingAvg, momentum;
        double upperThreshold, lowerThreshold;
    };  

    unordered_map<string, vector<MarketData>> loadMarketData(const vector<string>& companySymbols) {
        unordered_map<string, vector<MarketData>> allData;

        for (const auto& symbol : companySymbols) {
            vector<MarketData> dataForCompany = loadCompanyData(symbol + ".csv");
            if (!dataForCompany.empty()) {
                allData[symbol] = dataForCompany;
            } else {
                cout << "Warning: No data loaded for " << symbol << endl;
            }
        }

        return allData;
    }

private:
    vector<MarketData> loadCompanyData(const string& filename) {
        vector<MarketData> data;
        ifstream file(filename);
        if (!file.is_open()) {
            cout << "Error: Could not open file " << filename << endl;
            return data;
        }

        string line;
        // Skipping header line
        getline(file, line);
        
        while (getline(file, line)) {
            if (line.empty()) continue;

            MarketData entry;
            stringstream ss(line);
            string token;

            try {
                // Parse each value, separated by commas
                getline(ss, entry.date, ',');
                ss >> entry.openPrice;
                ss.ignore(); // to ignore the comma
                ss >> entry.highPrice;
                ss.ignore(); 
                ss >> entry.lowPrice;
                ss.ignore(); 
                ss >> entry.closePrice;
                ss.ignore(); 
                ss >> entry.volume;
                ss.ignore(); 
                ss >> entry.gain;
                ss.ignore(); 
                ss >> entry.loss;
                ss.ignore(); 
                ss >> entry.avgGain;
                ss.ignore(); 
                ss >> entry.avgLoss;
                ss.ignore(); 
                ss >> entry.rsi;
                ss.ignore(); 
                ss >> entry.movingAvg;
                ss.ignore(); 
                ss >> entry.momentum;
                ss.ignore(); 
                ss >> entry.upperThreshold;
                ss.ignore(); 
                ss >> entry.lowerThreshold;

                if (ss.fail()) {
                    cout << "Error: Malformed data in line: " << line << endl;
                    continue;
                }
            } catch (...) {
                cout << "Error reading data from line: " << line << endl;
                continue;
            }

            data.push_back(entry);
        }

        return data;
    }

public:
    double getLatestPrice(const string& symbol, const unordered_map<string, vector<MarketDataLoader::MarketData>>& marketData) {
        if (marketData.find(symbol) != marketData.end() && !marketData.at(symbol).empty()) {
            return marketData.at(symbol).back().closePrice;
        } else {
            cout << "No market data available for symbol: " << symbol << endl;
            return 0.0;
        }
    }
};

class Order {
protected:
    string symbol;
    int quantity;
    double price;

public:
    Order(string t, int qty, double p) : symbol(t), quantity(qty), price(p) {}
    virtual void execute(double currentPrice) = 0;
    string getSymbol() const { return symbol; }
    int getQuantity() const { return quantity; }
    double getPrice() const { return price; }
};

class MarketOrder : public Order {
public:
    MarketOrder(string symbol, int quantity) : Order(symbol, quantity, 0.0) {}
    void execute(double currentPrice) override {
        cout << "Executing Market Order: " << quantity << " shares of " << symbol << " at $" << currentPrice << endl;
        logTransaction("Market Order", symbol, quantity, currentPrice, "BUY"); // calling logTransaction() to store the MarketOrder
    }
};

class LimitOrder : public Order {
public:
    LimitOrder(string symbol, int quantity, double price) : Order(symbol, quantity, price) {}
    void execute(double currentPrice) override {
        if (currentPrice <= price) {
            cout << "Executing Limit Order: " << quantity << " shares of " << symbol << " at $" << price << endl;
            logTransaction("Limit Order", symbol, quantity, price, "BUY");
        } else {
            cout << "Limit Order for " << symbol << " not executed. Current price ($" << currentPrice 
                 << ") is higher than limit price ($" << price << ")." << endl;
        }
    }
};



class Stock {
public:
    string symbol;
    int quantity;
    double purchasePrice;

    // Default constructor
    Stock() : symbol(""), quantity(0), purchasePrice(0.0) {}

    // Parameterized constructor
    Stock(string t, int qty, double price) : symbol(t), quantity(qty), purchasePrice(price) {}

    // Copy constructor
    Stock(const Stock& other) : symbol(other.symbol), quantity(other.quantity), purchasePrice(other.purchasePrice) {}

    // // Move constructor (optional)
    // Stock(Stock&& other) noexcept : symbol(std::move(other.symbol)), quantity(other.quantity), purchasePrice(other.purchasePrice) {
    //     other.quantity = 0;
    //     other.purchasePrice = 0.0;
    // }
    // Copy assignment operator
    Stock& operator=(const Stock& other) {
        if (this == &other) return *this; // Handle self-assignment
        symbol = other.symbol;
        quantity = other.quantity;
        purchasePrice = other.purchasePrice;
        return *this;
    }

    string getSymbol() const { return symbol; }
    int getQuantity() const { return quantity; }
    double getPurchasePrice() const { return purchasePrice; }

    // Method to get the current value of the stock based on the current price
    double getCurrentValue(double currentPrice) const {
        return quantity * currentPrice;
    }
};

class Portfolio {
    unordered_map<string, Stock> stocks;
    double cashBalance;

public:
    Portfolio(double initialBalance = 100000) : cashBalance(initialBalance) {
        loadPortfolio();  // Load saved portfolio data at the start
    }

    // Save the portfolio state to file
    void savePortfolio() {
        ofstream file("portfolio.txt");
        if (!file.is_open()) {
            cout << "Error: Could not open portfolio file for saving." << endl;
            return;
        }

        // Save cash balance
        file << cashBalance << endl;

        // Save each stock: symbol, quantity, purchasePrice
        for (const auto& pair : stocks) { 
            file << pair.first << " " << pair.second.getQuantity() << " " << pair.second.getPurchasePrice() << endl;
        }

        file.close();
    }

    // Load the portfolio state from file
    void loadPortfolio() {
        ifstream file("portfolio.txt");
        if (!file.is_open()) {
            cout << "Portfolio file not found. Starting with a new portfolio." << endl;
            return;
        }

        // Load cash balance
        file >> cashBalance;

        // Load each stock: symbol, quantity, purchasePrice
        string symbol;
        int quantity;
        double price;
        while (file >> symbol >> quantity >> price) {
            stocks[symbol] = Stock(symbol, quantity, price);
        }

        file.close();
    }

    void addStock(const Stock& stock) {
        auto it = stocks.find(stock.getSymbol());
        if (it != stocks.end()) {
            it->second = Stock(stock.getSymbol(), it->second.getQuantity() + stock.getQuantity(), stock.getPurchasePrice());
        } else {
            stocks[stock.getSymbol()] = stock;
        }
        savePortfolio();  // Save portfolio after adding stock
    }

    bool buyStock(const string& symbol, int quantity, double price) {
        double totalCost = quantity * price;
        if (totalCost > cashBalance) {
            cout << "Insufficient cash balance to complete the purchase." << endl;
            return false;
        }

        cashBalance -= totalCost;
        addStock(Stock(symbol, quantity, price));
        // logTransaction("Buy", symbol, quantity, price, "BUY");
        savePortfolio();  // Save portfolio after buying stock
        cout << "Bought " << quantity << " shares of " << symbol << " at $" << price << endl;

        // Log transaction in "log_sell.txt"
        ofstream buyLogFile("log_buy.txt", ios::app);
        if (buyLogFile.is_open()) {
            buyLogFile << symbol << ", Quantity: " << quantity << ", Price: $" 
                        << price << ", Total: $" << quantity*price<< endl;
            buyLogFile.close();
        } else {
            cout << "Error opening sell log file." << endl;
        }
        return true;
    }


    void removeStock(const string& symbol, double currentPrice) {
        if (stocks.find(symbol) != stocks.end()) {
            double stockValue = stocks[symbol].getCurrentValue(currentPrice);
            cashBalance += stockValue;
            logTransaction("Sell", symbol, stocks[symbol].getQuantity(), currentPrice, "SELL");
            stocks.erase(symbol);
            cout << "Sold " << symbol << " for $" << stockValue << endl;
            savePortfolio();  // Save portfolio after removing stock
        } else {
            cout << "Stock " << symbol << " not found in portfolio." << endl;
        }
    }
    
    bool sellStock(const string& symbol, int quantity, double sellPrice) {
        auto it = stocks.find(symbol);
        if (it == stocks.end()) {
            cout << "Stock not found in portfolio: " << symbol << endl;
            return false;
        }

        Stock& stock = it->second;
        if (quantity > stock.getQuantity()) {
            cout << "Insufficient shares to sell. Available: " << stock.getQuantity() << endl;
            return false;
        }

        double saleProceeds = quantity * sellPrice;
        cashBalance += saleProceeds;

        // Update or remove stock
        if (quantity == stock.getQuantity()) {
            stocks.erase(it);
        } else {
            stock = Stock(symbol, stock.getQuantity() - quantity, stock.getPurchasePrice());
        }

        // Log transaction in "sell.txt"
        ofstream sellLogFile("log_sell.txt", ios::app);
        if (sellLogFile.is_open()) {
            sellLogFile << "Sell " << symbol << ", Quantity: " << quantity << ", Price: $" 
                        << sellPrice << ", Total: $" << saleProceeds << endl;
            sellLogFile.close();
        } else {
            cout << "Error opening sell log file." << endl;
        }

        savePortfolio();  // Save updated portfolio
        cout << "Sold " << quantity << " shares of " << symbol << " at $" << sellPrice << endl;
        return true;
    }

    double getPortfolioValue(const unordered_map<string, double>& currentPrices) {
        double totalValue = cashBalance;
        for (const auto& pair : stocks) {
            totalValue += pair.second.getCurrentValue(currentPrices.at(pair.first));
        }
        return totalValue;
    }

    void printPortfolio() {
        cout << "Portfolio Summary:" << endl;
        cout << "Cash Balance: $" << cashBalance << endl;
        for (const auto& pair : stocks) {
            cout << pair.first << " - Quantity: " << pair.second.getQuantity()
                 << " - Purchase Price: $" << pair.second.getPurchasePrice() << endl;
        }
    }
};




class TradingStrategy {
public:
    virtual void applyStrategy(const unordered_map<string, vector<MarketDataLoader::MarketData>>& marketData) = 0;
};

class MovingAverageStrategy : public TradingStrategy {
    int period;
public:
    MovingAverageStrategy(int p) : period(p) {}

    // Override the applyStrategy method from TradingStrategy
    void applyStrategy(const unordered_map<string, vector<MarketDataLoader::MarketData>>& marketData) override {
        cout << "Applying Moving Average Strategy..." << endl;
        
        for (const auto& entry : marketData) {
            const string& symbol = entry.first;
            const vector<MarketDataLoader::MarketData>& data = entry.second;
            
            if (data.size() < period) {
                cout << "Not enough data to calculate moving average for " << symbol << endl;
                continue;
            }

            // Calculate the moving average
            double sum = 0.0;
            for (int i = data.size() - period; i < data.size(); ++i) {
                sum += data[i].closePrice;
            }
            double movingAvg = sum / period;
            double latestPrice = data.back().closePrice;

            if (latestPrice < movingAvg) {
                cout << "Price is below moving average for " << symbol << ". Consider buying at price: " << latestPrice << endl;
            } else if (latestPrice > movingAvg) {
                cout << "Price is above moving average for " << symbol << ". Consider selling at price: " << latestPrice << endl;
            } else {
                cout << "Price is at moving average for " << symbol << ". No action needed." << endl;
            }
        }
    }
};


class RSIStrategy : public TradingStrategy {
    int period;
    double buyThreshold;
    double sellThreshold;

public:
    RSIStrategy(int p, double buyTh, double sellTh)
        : period(p), buyThreshold(buyTh), sellThreshold(sellTh) {}

    // Override the applyStrategy method from TradingStrategy
    void applyStrategy(const unordered_map<string, vector<MarketDataLoader::MarketData>>& marketData) override {
        cout << "Applying RSI Strategy..." << endl;

        for (const auto& entry : marketData) {
            const string& symbol = entry.first;
            const vector<MarketDataLoader::MarketData>& data = entry.second;

            if (data.size() < period) {
                cout << "Not enough data to calculate RSI for " << symbol << endl;
                continue;
            }

            // Calculate RSI (we assume the last element contains the latest RSI value)
            double latestRSI = data.back().rsi;

            // Signal based on RSI thresholds
            if (latestRSI < buyThreshold) {
                cout << "RSI below threshold. Consider buying " << symbol << " at RSI: " << latestRSI << endl;
            } else if (latestRSI > sellThreshold) {
                cout << "RSI above threshold. Consider selling " << symbol << " at RSI: " << latestRSI << endl;
            } else {
                cout << "RSI is within neutral range for " << symbol << " at RSI: " << latestRSI << endl;
            }
        }
    }
};

class MeanReversionStrategy : public TradingStrategy {
    int period;
    double deviationThreshold;

public:
    MeanReversionStrategy(int p, double deviation)
        : period(p), deviationThreshold(deviation) {}

    // Override the applyStrategy method from TradingStrategy
    void applyStrategy(const unordered_map<string, vector<MarketDataLoader::MarketData>>& marketData) override {
        cout << "Applying Mean Reversion Strategy..." << endl;

        for (const auto& entry : marketData) {
            const string& symbol = entry.first;
            const vector<MarketDataLoader::MarketData>& data = entry.second;

            if (data.size() < period) {
                cout << "Not enough data to calculate mean for " << symbol << endl;
                continue;
            }

            double sum = 0.0;
            for (int i = data.size() - period; i < data.size(); ++i) {
                sum += data[i].closePrice;
            }
            double movingAvg = sum / period;
            double latestPrice = data.back().closePrice;
            double deviation = (latestPrice - movingAvg) / movingAvg;

            // Signal based on deviation from the mean
            if (deviation < -deviationThreshold) {
                cout << "Price significantly below moving average for " << symbol << ". Consider buying at price: " << latestPrice << endl;
            } else if (deviation > deviationThreshold) {
                cout << "Price significantly above moving average for " << symbol << ". Consider selling at price: " << latestPrice << endl;
            } else {
                cout << "Price is within the normal range for " << symbol << ". No action needed." << endl;
            }
        }
    }
};

class MomentumStrategy : public TradingStrategy {
    int momentumPeriod;
public:
    MomentumStrategy(int period) : momentumPeriod(period) {}

    // Override the applyStrategy method to analyze momentum and output buy/sell signals
    void applyStrategy(const unordered_map<string, vector<MarketDataLoader::MarketData>>& marketData) override {
        cout << "Applying Momentum Strategy..." << endl;

        for (const auto& entry : marketData) {
            const string& symbol = entry.first;
            const vector<MarketDataLoader::MarketData>& data = entry.second;

            if (data.size() < momentumPeriod) {
                cout << "Not enough data to calculate momentum for " << symbol << endl;
                continue;
            }

            // Calculate momentum: compare the latest price with the price 'momentumPeriod' days ago
            double latestPrice = data.back().closePrice;
            double previousPrice = data[data.size() - momentumPeriod].closePrice;
            double momentum = latestPrice - previousPrice;

            // Signal generation based on momentum
            if (momentum > 0) {
                cout << "Positive momentum for " << symbol << ". Consider buying at price: " << latestPrice << endl;
            } else if (momentum < 0) {
                cout << "Negative momentum for " << symbol << ". Consider selling at price: " << latestPrice << endl;
            } else {
                cout << "No momentum change for " << symbol << ". No action needed." << endl;
            }
        }
    }
};


class TradeEngine {
    MarketDataLoader& loader;
    Portfolio& portfolio;

public:
    TradeEngine(MarketDataLoader& ld, Portfolio& pf) : loader(ld), portfolio(pf) {}

    void executeOrder(Order* order, double currentPrice) {
        if (MarketOrder* marketOrder = dynamic_cast<MarketOrder*>(order)) {
            // Execute market order and buy stock
            marketOrder->execute(currentPrice);
            portfolio.buyStock(marketOrder->getSymbol(), marketOrder->getQuantity(), currentPrice);
        } else if (LimitOrder* limitOrder = dynamic_cast<LimitOrder*>(order)) {
            // Execute limit order and buy stock if conditions met
            if (currentPrice <= limitOrder->getPrice()) {
                limitOrder->execute(currentPrice);
                portfolio.buyStock(limitOrder->getSymbol(), limitOrder->getQuantity(), currentPrice);
            } else {
                cout << "Limit Order for " << limitOrder->getSymbol() << " not executed. Price too high.\n";
            }
        }
    }

    void executeStrategy(TradingStrategy* strategy, const unordered_map<string, vector<MarketDataLoader::MarketData>>& marketData) {
        strategy->applyStrategy(marketData);
    }

    void MarketSell(const string& symbol, int quantity, double currentPrice) {
        portfolio.sellStock(symbol, quantity, currentPrice);
        logTransaction("Market Sell", symbol, quantity, currentPrice, "SELL");  // Log transaction
    }

    void LimitSell(const string& symbol, int quantity, double limitPrice, double currentPrice) {
        if (currentPrice >= limitPrice) {
            portfolio.sellStock(symbol, quantity, limitPrice);
            logTransaction("Limit Sell", symbol, quantity, limitPrice, "SELL");  // Log transaction
            cout << "Executed Limit Sell Order for " << quantity << " shares of " << symbol 
                 << " at $" << limitPrice << endl;
        } else {
            cout << "Limit Sell Order not executed. Current price $" << currentPrice 
                 << " is below limit price $" << limitPrice << endl;
        }
    }

};


// User interaction with the system
void displayMenu() {
    cout << "\n---- AutoTrader++ ----\n";
    cout << "1. View Portfolio\n";
    // cout << "2. View Latest Price of a Stock\n";
    cout << "2. View Current Market\n";
    cout << "3. Place a Market Order\n";
    cout << "4. Place a Limit Order\n";
    cout << "5. Execute a Trading Strategy\n";
    cout << "6. Sell a Market Order\n";
    cout << "7. Sell a Limit Order\n";
    cout << "8. Exit\n";
    cout << "Enter your choice: ";
}

// *********************************************
void parseLogLine(const string& logLine, string& transactionType, string& orderType, string& symbol, int& quantity, double& price, double& totalValue) {
    stringstream ss(logLine);
    string token;

    getline(ss, transactionType, ',');
    getline(ss, orderType, ',');
    getline(ss, symbol, ',');
    
    getline(ss, token, ',');
    quantity = stoi(token);
    
    getline(ss, token, ',');
    price = stod(token);
    
    getline(ss, token, ',');
    totalValue = stod(token);
}


void calculateTotalBuySell() {
    ifstream logFile("log.txt");
    if (!logFile.is_open()) {
        cout << "Error: Could not open transaction log file." << endl;
        return;
    }

    string logLine;
    double totalBuyValue = 0.0;
    double totalSellValue = 0.0;

    while (getline(logFile, logLine)) {
        string transactionType, orderType, symbol;
        int quantity;
        double price, totalValue;

        parseLogLine(logLine, transactionType, orderType, symbol, quantity, price, totalValue);

        if (transactionType == "BUY") {
            totalBuyValue += totalValue;
        } else if (transactionType == "SELL") {
            totalSellValue += totalValue;
        }
    }

    logFile.close();

    cout << "Total Buy Value: $" << totalBuyValue << endl;
    cout << "Total Sell Value: $" << totalSellValue << endl;
    cout<< "Total Buy-Sell Value: $" << totalBuyValue - totalSellValue << endl;
}

void calculateTotalProfitLoss() {
    vector<string> companies = {"AAPL", "GOOG", "AMZN", "TSLA", "MSFT","BABA","DIS","META","NFLX","NVDA"};
    MarketDataLoader loader;
    unordered_map<string, vector<MarketDataLoader::MarketData>> marketData = loader.loadMarketData(companies);

    ifstream logFile("log.txt");
    if (!logFile.is_open()) {
        cout << "Error: Could not open transaction log file." << endl;
        return;
    }

    string logLine;
    double totalProfit = 0.0;
    double totalLoss = 0.0;

    while (getline(logFile, logLine)) {
        string transactionType, orderType, symbol;
        int quantity;
        double price, totalValue;

        parseLogLine(logLine, transactionType, orderType, symbol, quantity, price, totalValue);

        if (transactionType == "SELL") {
            double currentPrice = loader.getLatestPrice(symbol,marketData);
            double profitLoss = totalValue - quantity * currentPrice;
            if (profitLoss > 0) {
                totalProfit += profitLoss;
            } else {
                totalLoss += profitLoss;
            }
        }
    }

    logFile.close();

    cout << "Total Profit: $" << totalProfit << endl;
    cout << "Total Loss: $" << totalLoss << endl;
}

// *****************************************************************************

int main() {
    vector<string> companies = {"AAPL", "GOOG", "AMZN", "TSLA", "MSFT","BABA","DIS","META","NFLX","NVDA"};
    MarketDataLoader loader;
    unordered_map<string, vector<MarketDataLoader::MarketData>> marketData = loader.loadMarketData(companies);

    Portfolio portfolio(100000); // Initial balance
    TradeEngine engine(loader, portfolio);

    // Displaying the menu
    int choice;
    do {
        displayMenu();
        cin >> choice;

        switch (choice) {
            case 1: {
                portfolio.printPortfolio();
                
                

                break;
            }
            case 2: {
                // cout<<"1. AAPL"<<endl;
                // cout<<"2. AMZN"<<endl;
                // cout<<"3. BABA"<<endl;
                // cout<<"4. DIS"<<endl;
                // cout<<"5. FB"<<endl;
                // cout<<"6. GOOG"<<endl;
                // cout<<"7. INTC"<<endl;
                // cout<<"8. META"<<endl;
                // cout<<"9. MSFT"<<endl;
                // cout<<"10. NFLX"<<endl;
                // cout<<"11. NVDA"<<endl;
                // cout<<"12. TSLA"<<endl;
                // cout << "Enter stock symbol: ";
                for(auto it: companies)
                {
                    cout<<it<<" : $"<<loader.getLatestPrice(it,marketData)<<endl;
                }
                // string symbol;
                // cin >> symbol;
                // double currentPrice = loader.getLatestPrice(symbol, marketData);
                // cout << "Latest price for " << symbol << ": $" << currentPrice << endl;

                
                break;
            }
            case 3: {
                cout << "Enter stock symbol: ";
                string symbol;
                int quantity;
                cin >> symbol;
                cout << "Enter quantity: ";
                cin >> quantity;

                double currentPrice = loader.getLatestPrice(symbol, marketData);
                MarketOrder marketOrder(symbol, quantity);
                engine.executeOrder(&marketOrder, currentPrice);
                
                break;
            }
            case 4: {
                cout << "Enter stock symbol: ";
                string symbol;
                int quantity;
                double price;
                cin >> symbol;
                double currentPrice = loader.getLatestPrice(symbol, marketData);
                cout<<"Current share price of "<<symbol<<" : "<<currentPrice<<endl;
                cout << "Enter limit price: ";
                cin >> price;
                cout << "Enter quantity: ";
                cin >> quantity;
                

                LimitOrder limitOrder(symbol, quantity, price);
                engine.executeOrder(&limitOrder, currentPrice);
                
                break;
            }
            case 5: {
                cout << "Choose strategy" << endl
                    << "1: Moving Average" << endl
                    << "2: RSI" << endl
                    << "3: Mean Reversion" << endl
                    << "4: Momentum" << endl
                    << "Enter your choice: ";
                int strategyChoice;
                cin >> strategyChoice;

                if (strategyChoice == 1) {
                    MovingAverageStrategy maStrategy(10); // 10-day moving average
                    engine.executeStrategy(&maStrategy, marketData);
                }
                else if (strategyChoice == 2) {
                    RSIStrategy rsiStrategy(14, 30.0, 70.0); // 14-day RSI, buy threshold 30, sell threshold 70
                    engine.executeStrategy(&rsiStrategy, marketData);
                }
                else if (strategyChoice == 3) {
                    MeanReversionStrategy mrStrategy(10, 0.05); // 10-day moving average, 5% deviation threshold
                    engine.executeStrategy(&mrStrategy, marketData);
                }
                else if (strategyChoice == 4) {
                    MomentumStrategy momentumStrategy(10); // 10-day momentum period
                    engine.executeStrategy(&momentumStrategy, marketData);
                }
                else {
                    cout << "Invalid strategy choice." << endl;
                }

                break;
            }
            case 6: {
                cout << "Enter stock symbol to sell: ";
                string symbol;
                int quantity;
                cin >> symbol;
                cout << "Enter quantity to sell: ";
                cin >> quantity;

                double currentPrice = loader.getLatestPrice(symbol, marketData);
                engine.MarketSell(symbol, quantity, currentPrice);
                
                break;
            }
            case 7: {
                cout << "Enter stock symbol to sell: ";
                string symbol;
                int quantity;
                double price;
                cin >> symbol;
                cout << "Enter quantity to sell: ";
                cin >> quantity;
                cout << "Enter limit price: ";
                cin >> price;

                double currentPrice = loader.getLatestPrice(symbol, marketData);
                engine.LimitSell(symbol, quantity, price, currentPrice);
                
                break;
            }
            case 8:
                cout << "Exiting..." << endl;
                exit(0);
                break;
            default:
                cout << "Invalid choice! Try again.\n";
        }
        while(1){
            cout<<endl;
            cout<<"Enter 0 to display menu again or enter 8 to exit"<<endl;
            string flag;
            cin>>flag;
            if(flag=="0") break;
            else if(flag=="8"){
                cout<<"Exiting...Bye!";
                exit(0);
            }
            else{
                cout<<"Invalid Choice, Try Again !!"<<endl;
            }
        }
    } while (choice != 8);

    return 0;
}
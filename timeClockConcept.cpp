/*
================================================================================
Employee Time Management System (prototype)

Author: Brayden Crosen
Version: 1.0

DESCRIPTION:
This project is a console-based employee time clock and management system written
in C++. It simulates a punch clock environment with permission-level-based access
control for associates, managers, and managers (master access). The system
tracks employee time status, permissions, and pay while saving all data across
program runs using text files.

FEATURES:
- Employee login using 7-digit personnel numbers
- Clock In / Clock Out functionality
- Start and End Meal tracking
- Automatic timestamping of punches to punchRecords.txt
- Employee data storage to employees.txt
- Role-based permissions:
    * Associate
    * Manager
    * Manager (master-access)
- Manager PIN verification for restricted actions
- View currently clocked-in and on-meal employees
- Add, remove, and edit employees
- Change employee pay with permission enforcement
- Promote/demote employees and manage master access
- Input validation to prevent invalid or unsafe operations

PERMISSIONS MODEL:
- Managers (listed as MGR) can:
    * Change associates' pay
    * Promote associates to managers
    * View clocked-in
    * Add / remove associates
- Managers with master access (listed as MGR*) can also:
    * Change all employees' pay
    * Demote associates
    * Add / remove employees

TODO:
I plan on rewriting this project and integrating ncurses for a more realistic display
refresh. Evidently, I'd like to convert this code using the same logic in python
to an application using tkinter, but that's further down the line. 

GETTING STARTED:
To log in as a test user, enter the ID: 1111111

To create your own profile:

Log in as test user -> Edit employee info (7) -> Enter manager pin: 1111 -> Add (1) -> Enter name -> Create 7 digit ID# -> Enter pay -> Assign manager (1, recommended) -> Assign master permission (1, recommended) -> Create 4 digit manager pin -> Exit (5)

Once the new profile has been created, it will be saved to employees.txt

To reset all information, simply clear or delete the employees.txt file and the program will auto populate 4 employees and a test user.
================================================================================
*/
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <ctime>
#include <fstream>
#include <limits>

using namespace std;

// Employee class definition
class employee
{
private:
    string name;
    int employeeID;
    double pay;
    bool isManager;
    int managerPin;
    bool masterStatus;
    int timeStatus; // 0 (off clock) | 1 (on clock) | 2 (on meal)

public:
    employee(string empName, int empID, double empPay, bool managerStatus, int pin, bool mststatus, int status)
    {
        name = empName;
        employeeID = empID;
        pay = empPay;
        isManager = managerStatus;
        managerPin = pin;
        masterStatus = mststatus;
        timeStatus = status;
    }
    // Getter functions
    string getName() const { return name; }
    int getID() const { return employeeID; }
    double getPay() const { return pay; }
    bool getMgrStatus() const { return isManager; }
    int getMgrPin() const { return managerPin; }
    bool getMstrStatus() const { return masterStatus; }
    int getStatus() const { return timeStatus; }
    // Setter functions
    void setTimeStatus(int set) { timeStatus = set; }
    void setPay(double newPay) { pay = newPay; }
    void setPin(int pin) { managerPin = pin; }
    void setPermissions(int status)
    {
        if (status == 0)
        {
            isManager = true;
            masterStatus = false;
        }
        else if (status == 1)
        {
            isManager = true;
            masterStatus = true;
        }
        else
        {
            isManager = false;
            masterStatus = false;
        }
    }
};

struct punch
{
    int employeeID;
    string name;
    string type;
    string timestamp;
};

// Save employees to .txt file
void saveEmployees(const vector<employee> &employees)
{
    ofstream file("employees.txt");
    for (const auto &e : employees)
    {
        file << e.getName() << "|"
             << e.getID() << "|"
             << e.getPay() << "|"
             << e.getMgrStatus() << "|"
             << e.getMgrPin() << "|"
             << e.getMstrStatus() << "|"
             << e.getStatus() << "\n";
    }
}

// Load employee data from .txt file
void loadEmployees(vector<employee> &employees)
{
    ifstream file("employees.txt");
    if (!file.is_open())
        return;

    employees.clear();
    string line;

    while (getline(file, line))
    {
        size_t p[6];
        p[0] = line.find("|");
        for (int i = 1; i < 6; i++)
            p[i] = line.find("|", p[i - 1] + 1);

        if (p[5] == string::npos)
            continue;

        string name = line.substr(0, p[0]);
        int id = stoi(line.substr(p[0] + 1, p[1] - p[0] - 1));
        double pay = stod(line.substr(p[1] + 1, p[2] - p[1] - 1));
        bool mgr = stoi(line.substr(p[2] + 1, p[3] - p[2] - 1));
        int pin = stoi(line.substr(p[3] + 1, p[4] - p[3] - 1));
        bool master = stoi(line.substr(p[4] + 1, p[5] - p[4] - 1));
        int status = stoi(line.substr(p[5] + 1));

        employees.push_back(employee(name, id, pay, mgr, pin, master, status));
    }
}

// Save punches to .txt file
void savePunch(const punch &p)
{
    ofstream file("punchRecords.txt", ios::app);
    file << p.employeeID << "--" << p.name << "--" << p.type << "--" << p.timestamp << "\n";
}

// Return current time
string getTime()
{
    time_t now = time(0);
    tm *ltm = localtime(&now);

    char buffer[40];
    strftime(buffer, sizeof(buffer), "%D %H:%M:%S", ltm);

    return string(buffer);
}

// Display header
void printHeader(int &employeeidx, vector<employee> &employees)
{
    cout << endl;
    cout << "Employee Time Management System\n";
    cout << getTime() << endl;

    if (employeeidx > -1)
    {
        if (employees[employeeidx].getMstrStatus())
            cout << "**";

        cout << employees[employeeidx].getName();

        if (employees[employeeidx].getMstrStatus())
            cout << "**";

        cout << "\n";
    }
}

// Validate login ID
bool checkLoginInput(vector<employee> &employees, int &id)
{
    // Check size
    if (id < 1000000 || id > 9999999)
    {
        cout << "Your personnel # must be 7 digits" << endl;
        return false;
    }

    for (int i = 0; i < employees.size(); i++)
    {
        if (employees[i].getID() == id)
            return true;
    }

    cout << "personnel # not found" << endl;
    return false;
}

int employeeLogin(vector<employee> &employees)
{
    while (true)
    {
        cout << "Enter your personnel #: ";
        int id;
        cin >> id;

        // Check for cin faiure
        if (cin.fail())
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Your ID must be numeric" << endl;
            continue;
        }

        if (checkLoginInput(employees, id))
            return id;
    }
    return 0;
}

// Set the employee index
int setIndex(int input, vector<employee> &employees)
{
    for (int i = 0; i < employees.size(); i++)
        if (input == employees[i].getID())
            return i;
    return -1;
}

// Display employee menu
char employeeMenu(vector<employee> &employees, int &employeeidx)
{
    int ubound = employees[employeeidx].getMgrStatus() ? 8 : 6;

    // Main menu
    cout << "1 - Clock In\n"
         << "2 - Clock Out\n"
         << "3 - Start Meal\n"
         << "4 - End Meal\n"
         << "5 - Show Last Punch\n";

    // Manager menu extension
    if (employees[employeeidx].getMgrStatus())
    {
        cout << "6 - View Clocked In\n"
             << "7 - Edit Employee Info\n"
             << "8 - Cancel\n";
    }
    else
    {
        cout << "6 - Cancel\n";
    }

    char choice;
    while (true)
    {
        cout << "-> ";
        cin >> choice;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (choice >= '1' && choice <= '0' + ubound)
            return choice;

        cout << "Invalid, try again\n";
    }
}

// Verify manager pin
bool verifyPin(vector<employee> &employees, int &employeeidx)
{
    int pin;
    while (true)
    {
        cout << "Enter manager pin: ";
        cin >> pin;
        // Check numeric
        if (cin.fail())
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Your manager pin must be numeric" << endl;
            continue;
        }
        // Check size
        if (pin < 1000 || pin > 9999)
        {
            cout << "Your pin must be 4 digits" << endl;
            continue;
        }
        // Check match
        if (employees[employeeidx].getMgrPin() == pin)
        {
            return true;
        }
        else
        {
            cout << "Incorrect, logging you out" << endl;
            return false;
        }
    }
}

// USER MENU FUNCTIONS
void clockIn(vector<employee> &employees, int &employeeidx)
{
    // Prevent clock in if on clock
    if (employees[employeeidx].getStatus() == 0)
    {
        // Create p struct and pass to .txt file
        punch p{employees[employeeidx].getID(), employees[employeeidx].getName(), "CLOCK_IN", getTime()};
        savePunch(p);
        employees[employeeidx].setTimeStatus(1);
        cout << endl
             << employees[employeeidx].getName() << ", you are now clocked in at " << getTime() << endl;
    }
    else if (employees[employeeidx].getStatus() == 2)
    {
        cout << "\nYou are on a meal break, select end meal" << endl;
    }
    else
    {
        cout << "\nYou are already clocked in" << endl;
    }
}

void clockOut(vector<employee> &employees, int &employeeidx)
{
    // Prevent clock out if on meal or not on clock
    if (employees[employeeidx].getStatus() == 1)
    {
        punch p{employees[employeeidx].getID(), employees[employeeidx].getName(), "CLOCK_OUT", getTime()};
        savePunch(p);
        employees[employeeidx].setTimeStatus(0);
        cout << endl
             << employees[employeeidx].getName() << ", you are now clocked out at " << getTime() << endl;
    }
    else
    {
        cout << "\nYou are not clocked in" << endl;
    }
}

void startMeal(vector<employee> &employees, int &employeeidx)
{
    // Prevent start meal if not clocked in or already on meal
    if (employees[employeeidx].getStatus() == 1)
    {
        punch p{employees[employeeidx].getID(), employees[employeeidx].getName(), "START_MEAL", getTime()};
        savePunch(p);
        employees[employeeidx].setTimeStatus(2);
        cout << endl
             << employees[employeeidx].getName() << ", start meal saved at " << getTime() << endl;
    }
    else
    {
        cout << "\nYou are not clocked in" << endl;
    }
}

void endMeal(vector<employee> &employees, int &employeeidx)
{
    // Prevent end meal if not on a meal or not clocked in
    if (employees[employeeidx].getStatus() == 2)
    {
        punch p{employees[employeeidx].getID(), employees[employeeidx].getName(), "END_MEAL", getTime()};
        savePunch(p);
        employees[employeeidx].setTimeStatus(1);
        cout << endl
             << employees[employeeidx].getName() << ", end meal saved at " << getTime() << endl;
    }
    else
    {
        cout << "\nYou are not on a meal" << endl;
    }
}

punch getLastPunch(int employeeID)
{
    ifstream file("punchRecords.txt");
    string line;
    punch last = {0, "", "", ""};

    while (getline(file, line))
    {
        size_t p1 = line.find("--");
        size_t p2 = line.find("--", p1 + 2);
        size_t p3 = line.find("--", p2 + 2);

        if (p1 == string::npos || p2 == string::npos || p3 == string::npos)
            continue;

        int idFromFile = stoi(line.substr(0, p1));

        if (idFromFile == employeeID)
        {
            last.employeeID = idFromFile;
            last.name = line.substr(p1 + 2, p2 - (p1 + 2));
            last.type = line.substr(p2 + 2, p3 - (p2 + 2));
            last.timestamp = line.substr(p3 + 2);
        }
    }

    return last;
}

// INVISIBLE MANAGER FUNCTIONS
void displayEmployees(vector<employee> &employees, int &employeeidx)
{
    cout << "\nEMPLOYEES:\n";
    for (int i = 0; i < employees.size(); i++)
    {
        // Display list
        cout << left;
        // Hide manager id (show own id)
        if (!employees[employeeidx].getMstrStatus() && employees[i].getMgrStatus() && (employees[i].getID() != employees[employeeidx].getID()))
        {
            cout << setw(9) << "*******";
        }
        else
        {
            cout << setw(9) << employees[i].getID();
        }
        cout << setw(20) << employees[i].getName()
             << "$" << setw(9) << fixed << setprecision(2) << employees[i].getPay();
        if (employees[i].getMgrStatus())
            cout << "MGR";
        if (employees[i].getMstrStatus())
            cout << "*";
        cout << "\n";
    }
}

void addEmployee(vector<employee> &employees, int &employeeidx)
{
    string name;
    int id;
    double pay;
    int mgrInput;
    bool isManager;
    int mgrpin = 0;
    int mstInput;
    bool isMaster;

    cin.ignore(numeric_limits<streamsize>::max(), '\n'); // FIX getline issue

    // Name
    cout << "Enter name: ";
    getline(cin, name);

    // ID
    while (true)
    {
        cout << "Enter personnel #: ";
        cin >> id;

        if (cin.fail() || id < 1000000 || id > 9999999)
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "ID must be a 7-digit number\n";
            continue;
        }

        // Check duplicate ID
        bool exists = false;
        for (int i = 0; i < employees.size(); i++)
        {
            if (employees[i].getID() == id)
            {
                cout << "ID already exists" << endl;
                exists = true;
                break;
            }
        }

        if (!exists)
            break;
    }

    // Pay
    while (true)
    {
        cout << "Enter pay: ";
        cin >> pay;

        if (cin.fail() || pay < 0)
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Pay must be a positive number\n";
            continue;
        }
        break;
    }

    // Manager status (only accessible to employees with master access)
    if (employees[employeeidx].getMstrStatus())
    {
        while (true)
        {
            cout << "Enter 0 for associate or 1 for manager: ";
            cin >> mgrInput;

            if (cin.fail() || (mgrInput != 0 && mgrInput != 1))
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Enter only 0 or 1\n";
                continue;
            }

            isManager = (mgrInput == 1);
            break;
        }
    }

    // Master status (only accessible to employees with master access)
    if (employees[employeeidx].getMstrStatus() && isManager)
    {
        while (true)
        {
            cout << "Enter 0 to continue or 1 to grant master access: ";
            cin >> mstInput;

            if (cin.fail() || (mstInput != 0 && mstInput != 1))
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Enter only 0 or 1\n";
                continue;
            }

            isMaster = (mstInput == 1);
            break;
        }
    }
    else
    {
        isMaster = false;
    }

    // Manager pin (only if manager)
    if (isManager)
    {
        while (true)
        {
            cout << "Enter 4-digit manager pin: ";
            cin >> mgrpin;

            if (cin.fail() || mgrpin < 1000 || mgrpin > 9999)
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Pin must be 4 digits\n";
                continue;
            }
            break;
        }
    }

    employees.push_back(employee(name, id, pay, isManager, mgrpin, isMaster, 0));
    saveEmployees(employees);

    cout << "\nEmployee added successfully.\n";
}

void removeEmployee(vector<employee> &employees, int &employeeidx)
{
    int id;
    int idx;
    bool found = false;

    while (true)
    {
        cout << "Enter employee #: ";
        cin >> id;

        // Verify input type and isze
        if (cin.fail() || id < 1000000 || id > 9999999)
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "ID must be a 7-digit number\n";
            continue;
        }

        // Prevent self deletion
        if (id == employees[employeeidx].getID())
        {
            cout << "\nYou may not remove yourself as an employee" << endl;
            return;
        }

        // Set removal index
        for (int i = 0; i < employees.size(); i++)
        {
            if (employees[i].getID() == id)
            {
                idx = i;
                found = true;
            }
        }

        // Not found
        if (!found)
        {
            cout << "Employee # not found" << endl;
            continue;
        }

        // Prevent manager removal without master access
        if (!employees[employeeidx].getMstrStatus() && employees[idx].getMgrStatus())
        {
            cout << "\nYou must have master access to remove a manager" << endl;
            return;
        }

        // Remove
        if (found)
        {
            cout << "\n"
                 << employees[idx].getName() << " has been removed" << endl;
            employees.erase(employees.begin() + idx);
            saveEmployees(employees);
            return;
        }
    }
}

void changePay(vector<employee> &employees, int &employeeidx)
{
    int id;
    int idx = -1;

    cout << "Enter personnel #: ";
    cin >> id;

    // Check id
    if (cin.fail() || id < 1000000 || id > 9999999)
    {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "ID must be a 7-digit number\n";
        return;
    }

    // Prevent self-pay change
    if (id == employees[employeeidx].getID())
    {
        cout << "\nYou cannot change your own pay\n";
        return;
    }

    // Find employee
    for (int i = 0; i < employees.size(); i++)
    {
        if (employees[i].getID() == id)
        {
            idx = i;
            break;
        }
    }

    if (idx == -1)
    {
        cout << "Personnel # not found\n";
        return;
    }

    // Cannot change master access pay without having master access
    if (employees[idx].getMstrStatus() && !employees[employeeidx].getMstrStatus())
    {
        cout << "\nYou do not have permission to change this employee's pay\n";
        return;
    }

    double newPay;
    cout << "Enter new pay: ";
    cin >> newPay;

    if (cin.fail() || newPay < 0)
    {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Pay must be a positive number.\n";
        return;
    }

    cout << "\nPay updated: "
         << employees[idx].getName()
         << " ($" << fixed << setprecision(2)
         << employees[idx].getPay()
         << " to $" << newPay << ")\n";

    employees[idx].setPay(newPay);
    saveEmployees(employees);
}

int statusChangeCheck(vector<employee> &employees, int &employeeidx)
{
    int id;
    int idx = -1;
        cout << "Enter personnel #: ";
        cin >> id;

        // Check id
        if (cin.fail() || id < 1000000 || id > 9999999)
        {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "ID must be a 7-digit number\n";
            return -1;
        }

        // Prevent self-pay change
        if (id == employees[employeeidx].getID())
        {
            cout << "\nYou cannot change your own status\n";
            return -1;
        }

        // Find employee
        for (int i = 0; i < employees.size(); i++)
        {
            if (employees[i].getID() == id)
            {
                idx = i;
                break;
            }
        }

        if (idx == -1)
        {
            cout << "Personnel # not found\n";
            return -1;
            ;
        }

        // Cannot change master access status without having master access
        if (employees[idx].getMstrStatus() && !employees[employeeidx].getMstrStatus())
        {
            cout << "\nYou do not have permission to change this employee's status\n";
            return -1;
        }

        return idx;
}

void createMgrPin(vector<employee> &employees, int &employeeidx, int idx)
{
    int pin;
    // Create manager pin if one doesn't already exist
    if (employees[idx].getMgrPin() == 0)
    {
        while (true)
        {
            cout << "\nCreate manager pin: ";
            cin >> pin;
            // Check pin
            if (cin.fail() || pin < 1000 || pin > 9999)
            {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Pin must be a 4-digit number\n";
                continue;
            }
            employees[idx].setPin(pin);
            break;
        }
    }
}

void changeStatus(vector<employee> &employees, int &employeeidx)
{
    int idx = statusChangeCheck(employees, employeeidx);
    if (idx == -1)
        return;

    char choice;

    cout << "\nWould you like to:\n"
         << "1 - Promote to manager\n"
         << "2 - Demote to associate\n"
         << "3 - Grant master access\n"
         << "4 - Remove master access\n"
         << "-> ";
    cin >> choice;

    switch (choice)
    {
    // Promote to manager (Manager OR Master)
    case '1':
        if (employees[idx].getMgrStatus())
        {
            cout << "\nEmployee is already a manager\n";
            return;
        }

        createMgrPin(employees, employeeidx, idx);

        employees[idx].setPermissions(0);
        cout << "\nEmployee promoted to manager\n";
        saveEmployees(employees);
        break;

    // Demote to associate (MASTER ONLY)
    case '2':
        if (!employees[employeeidx].getMstrStatus())
        {
            cout << "\nYou do not have permission to demote employees\n";
            return;
        }

        if (!employees[idx].getMgrStatus())
        {
            cout << "\nThis employee is already an associate\n";
            return;
        }

        employees[idx].setPermissions(-1);
        cout << "\nEmployee demoted to associate.\n";
        employees[idx].setPin(0);
        break;

    // Grant master (MASTER ONLY)
    case '3':
        if (!employees[employeeidx].getMstrStatus())
        {
            cout << "\nYou do not have permission to grant master access\n";
            return;
        }

        if (employees[idx].getMstrStatus())
        {
            cout << "\nEmployee already has master access\n";
            return;
        }
        // Create manager pin if one does not already exist
        createMgrPin(employees, employeeidx, idx);

        employees[idx].setPermissions(1);
        cout << "\nMaster access granted.\n";
        saveEmployees(employees);
        break;

    // Remove master access (MASTER ONLY)
    case '4':
        if (!employees[employeeidx].getMstrStatus())
        {
            cout << "\nYou do not have permission to remove master access\n";
            return;
        }

        if (!employees[idx].getMstrStatus())
        {
            cout << "\nThis employee does not have master access\n";
            return;
        }
        cout << "\nEmployee no longer has master access\n";
        employees[idx].setPermissions(0);
        saveEmployees(employees);
        break;

    default:
        cout << "Invalid choice.\n";
    }
}

// MANAGER MENU FUNCTIONS
void editInfo(vector<employee> &employees, int &employeeidx)
{
    while (true)
    {
        displayEmployees(employees, employeeidx);

        cout << "\n";
        cout << "Would you like to:\n"
             << "1 - Add\n"
             << "2 - Remove\n"
             << "3 - Change pay\n"
             << "4 - Change Status\n"
             << "5 - Exit\n"
             << "->";
        char choice;
        cin >> choice;

        switch (choice)
        {
        // Add employee
        case '1':
            addEmployee(employees, employeeidx);
            break;

        // Remove employee
        case '2':
            removeEmployee(employees, employeeidx);
            break;

        // Change pay
        case '3':
            changePay(employees, employeeidx);
            break;

        // Change status
        case '4':
            changeStatus(employees, employeeidx);
            break;

        // Exit
        case '5':
            return;
            break;

        // Invalid
        default:
            cout << "Unknown, try again" << endl;
            break;
        }
    }
}

void viewClockedIn(vector<employee> &employees, int &employeeidx)
{
    bool clockedIn = false;
    // Show clocked in employees
    cout << "\n--Clocked In--" << endl;
    for (int i = 0; i < employees.size(); i++)
    {
        if (employees[i].getStatus() == 1)
        {
            cout << left << setw(20) << employees[i].getName();

            // Display manager status
            if (employees[i].getMgrStatus())
            {
                cout << "MGR";
            }
            if (employees[i].getMstrStatus())
            {
                cout << "*";
            }
            cout << "\n";
            clockedIn = true;
        }
    }
    if (!clockedIn)
    {
        cout << "\nNo employees are clocked in" << endl;
    }

    bool onMeal = false;
    // Check if employees are on meal
    for (int i = 0; i < employees.size(); i++)
    {
        if (employees[i].getStatus() == 2)
            onMeal = true;
    }

    if (onMeal)
    {
        cout << "\n--On Meal--" << endl;
        for (int i = 0; i < employees.size(); i++)
        {
            if (employees[i].getStatus() == 2)
            {
                cout << left << setw(20) << employees[i].getName();

                // Display manager status
                if (employees[i].getMgrStatus())
                {
                    cout << "MGR";
                }
                if (employees[i].getMstrStatus())
                {
                    cout << "*";
                }
                cout << "\n";
            }
        }
    }
}

// MAIN
int main()
{
    vector<employee> employees;

    loadEmployees(employees);

    if (employees.empty())
    {
        // (Name, ID, Pay, Mgr Status, Mgr Pin, Master Access, 0)
        employees.push_back(employee("Test User", 1111111, 20.00, true, 1111, true, 1));
        employees.push_back(employee("Alex Martinez", 2039485, 15.25, false, 0, false, 1));
        employees.push_back(employee("Samantha Lee", 4012346, 16.10, true, 2864, false, 2));
        employees.push_back(employee("Jordan Patel", 1964273, 15.75, false, 0, false, 2));
        employees.push_back(employee("Chris Donovan", 4012348, 17.00, false, 0, false, 1));

        saveEmployees(employees);
    }

    //// Master Session
    while (true)
    {
        int employeeidx = -1;
        saveEmployees(employees);
        loadEmployees(employees);
        printHeader(employeeidx, employees);
        // Display login screen and set index to ID
        int id = employeeLogin(employees);
        employeeidx = setIndex(id, employees);

        // Session
        while (true)
        {
            punch last;
            printHeader(employeeidx, employees);
            switch (employeeMenu(employees, employeeidx))
            {
            case '1':
                // Clock In
                clockIn(employees, employeeidx);
                break;

            case '2':
                // Clock Out
                clockOut(employees, employeeidx);
                break;

            case '3':
                // Start Meal
                startMeal(employees, employeeidx);
                break;

            case '4':
                // End Meal
                endMeal(employees, employeeidx);
                break;

            case '5':
            {
                // Show Last Punch
                last = getLastPunch(employees[employeeidx].getID());
                if (last.employeeID == 0)
                    cout << "No punches found.\n";
                else
                    cout << "\nLast punch: " << last.type
                         << " at " << last.timestamp << endl;
                break;
            }
            case '6':
                // Logout -- IF MANAGER, VIEW CLOCKED IN (does not verify pin)
                if (employees[employeeidx].getMgrStatus())
                    viewClockedIn(employees, employeeidx);
                break;

            case '7':
                // Edit info
                if (verifyPin(employees, employeeidx))
                    editInfo(employees, employeeidx);
                break;

            case '8':
                // Mgr logout
                break;
        }
            break;
        }
    }
}

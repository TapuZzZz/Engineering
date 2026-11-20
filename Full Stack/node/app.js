const mysql = require('mysql2/promise');
const readline = require('readline');
const express = require('express');
const morgan = require('morgan');

const app = express();
app.use(morgan('dev'));

app.get('/log', (req, res) => {
    res.send('Action logged!');
});

const server = app.listen(3000, () => {
    console.log("Background logging server running on port 3000");
});

const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
});

const askQuestion = (query) => {
    return new Promise((resolve) => rl.question(query, resolve));
};

async function main() {
    let connection;

    try {
        connection = await mysql.createConnection({
            host: 'localhost',
            user: 'TapuZ',
            password: 'TapuZ2007', 
            database: 'app_db'    
        });

        console.log("Successfully connected to the database.");

        let isRunning = true;

        while (isRunning) {
            console.log("\n--- System Menu ---");
            console.log("1. Search user by username");
            console.log("2. Add a new user");
            console.log("3. Delete a user");
            console.log("4. Exit");
            
            const choice = await askQuestion("Select an option (1-4): ");

            switch (choice) {
                case '1':
                    const searchUsername = await askQuestion("Enter username to search: ");
                    const [searchRows] = await connection.execute(
                        'SELECT * FROM users WHERE username = ?', 
                        [searchUsername]
                    );
                    console.log("Search Results:");
                    console.table(searchRows);
                    break;
                
                case '2':
                    const newUsername = await askQuestion("Enter new username: ");
                    const newEmail = await askQuestion("Enter email address: ");
                    await connection.execute(
                        'INSERT INTO users (username, email) VALUES (?, ?)', 
                        [newUsername, newEmail]
                    );
                    console.log("User added successfully.");
                    break;

                case '3':
                    const deleteUsername = await askQuestion("Enter username to delete: ");
                    await connection.execute(
                        'DELETE FROM users WHERE username = ?', 
                        [deleteUsername]
                    );
                    console.log("User deleted successfully.");
                    break;

                case '4':
                    isRunning = false;
                    console.log("Closing application...");
                    break;

                default:
                    console.log("Invalid option, please try again.");
            }

            if (isRunning) {
                await fetch(`http://localhost:3000/log?action=${choice}`).catch(() => {});
            }
        }

    } catch (error) {
        console.error("An error occurred:", error.message);
    } finally {
        if (connection) {
            await connection.end();
        }
        rl.close();
        server.close();
    }
}

main();
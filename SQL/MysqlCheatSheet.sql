-- =====================================================
-- MySQL Cheat Sheet
-- =====================================================

-- ---------- DATABASE ----------
CREATE DATABASE shop CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
SHOW DATABASES;
USE shop;
DROP DATABASE shop;

-- ---------- CREATE TABLE ----------
CREATE TABLE customers (
    id         INT AUTO_INCREMENT PRIMARY KEY,
    name       VARCHAR(100) NOT NULL,
    email      VARCHAR(150) UNIQUE,
    age        INT CHECK (age >= 0),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE orders (
    id          INT AUTO_INCREMENT PRIMARY KEY,
    customer_id INT NOT NULL,
    total       DECIMAL(10,2) NOT NULL,
    status      ENUM('new','paid','shipped') DEFAULT 'new',
    order_date  DATE,
    FOREIGN KEY (customer_id) REFERENCES customers(id)
        ON DELETE CASCADE ON UPDATE CASCADE
);

SHOW TABLES;
DESCRIBE customers;
SHOW CREATE TABLE customers;

-- ---------- ALTER / DROP ----------
ALTER TABLE customers ADD COLUMN phone VARCHAR(20);
ALTER TABLE customers MODIFY COLUMN phone VARCHAR(30);
ALTER TABLE customers RENAME COLUMN phone TO mobile;
ALTER TABLE customers DROP COLUMN mobile;
RENAME TABLE customers TO clients;
DROP TABLE IF EXISTS orders;
TRUNCATE TABLE orders;              -- delete all rows, reset AUTO_INCREMENT

-- ---------- INSERT ----------
INSERT INTO customers (name, email, age) VALUES ('Dana', 'dana@mail.com', 25);
INSERT INTO customers (name, email, age) VALUES
    ('Avi', 'avi@mail.com', 30),
    ('Noa', 'noa@mail.com', 22);
INSERT INTO customers (name, email) VALUES ('Dan', 'dan@mail.com')
    ON DUPLICATE KEY UPDATE name = VALUES(name);

-- ---------- SELECT ----------
SELECT * FROM customers;
SELECT name, age FROM customers WHERE age >= 18 AND age <> 30;
SELECT DISTINCT status FROM orders;
SELECT name AS customer_name FROM customers;
SELECT * FROM customers ORDER BY age DESC, name ASC LIMIT 10 OFFSET 20;

-- ---------- WHERE OPERATORS ----------
SELECT * FROM customers WHERE age BETWEEN 20 AND 30;
SELECT * FROM customers WHERE name LIKE 'D%';         -- starts with D
SELECT * FROM customers WHERE name LIKE '_a%';        -- 2nd letter a
SELECT * FROM customers WHERE age IN (22, 25, 30);
SELECT * FROM customers WHERE email IS NULL;
SELECT * FROM customers WHERE email IS NOT NULL;
SELECT * FROM customers WHERE NOT age = 25;

-- ---------- UPDATE / DELETE ----------
UPDATE customers SET age = 26, name = 'Dana L.' WHERE id = 1;
DELETE FROM customers WHERE id = 3;

-- ---------- AGGREGATES / GROUP BY ----------
SELECT COUNT(*), SUM(total), AVG(total), MIN(total), MAX(total) FROM orders;

SELECT customer_id, COUNT(*) AS num_orders, SUM(total) AS spent
FROM orders
WHERE status = 'paid'            -- filter rows BEFORE grouping
GROUP BY customer_id
HAVING SUM(total) > 500          -- filter groups AFTER grouping
ORDER BY spent DESC;

-- ---------- JOINS ----------
SELECT c.name, o.total
FROM customers c
INNER JOIN orders o ON o.customer_id = c.id;        -- only matches

SELECT c.name, o.total
FROM customers c
LEFT JOIN orders o ON o.customer_id = c.id;         -- all customers

SELECT c.name, o.total
FROM customers c
RIGHT JOIN orders o ON o.customer_id = c.id;        -- all orders

-- FULL OUTER JOIN emulation
SELECT c.name, o.total FROM customers c LEFT JOIN orders o ON o.customer_id = c.id
UNION
SELECT c.name, o.total FROM customers c RIGHT JOIN orders o ON o.customer_id = c.id;

-- customers with no orders
SELECT c.* FROM customers c
LEFT JOIN orders o ON o.customer_id = c.id
WHERE o.id IS NULL;

-- ---------- SUBQUERIES ----------
SELECT * FROM customers
WHERE id IN (SELECT customer_id FROM orders WHERE total > 100);

SELECT * FROM orders
WHERE total > (SELECT AVG(total) FROM orders);

SELECT * FROM customers c
WHERE EXISTS (SELECT 1 FROM orders o WHERE o.customer_id = c.id);

-- ---------- CTE ----------
WITH big_orders AS (
    SELECT customer_id, SUM(total) AS spent FROM orders GROUP BY customer_id
)
SELECT c.name, b.spent FROM big_orders b JOIN customers c ON c.id = b.customer_id;

-- ---------- WINDOW FUNCTIONS (MySQL 8+) ----------
SELECT id, customer_id, total,
       ROW_NUMBER() OVER (PARTITION BY customer_id ORDER BY total DESC) AS rn,
       RANK()       OVER (ORDER BY total DESC) AS rnk,
       SUM(total)   OVER (PARTITION BY customer_id) AS customer_total
FROM orders;

-- ---------- STRING FUNCTIONS ----------
SELECT CONCAT(name, ' - ', email), UPPER(name), LOWER(name), LENGTH(name),
       SUBSTRING(name, 1, 3), TRIM('  x  '), REPLACE(name, 'a', 'o')
FROM customers;

-- ---------- NUMBER / DATE FUNCTIONS ----------
SELECT ROUND(3.567, 2), CEIL(3.2), FLOOR(3.8), ABS(-5), MOD(10, 3);
SELECT NOW(), CURDATE(), CURTIME();
SELECT DATE_ADD(CURDATE(), INTERVAL 7 DAY), DATEDIFF('2026-12-31', CURDATE());
SELECT DATE_FORMAT(NOW(), '%d/%m/%Y %H:%i'), YEAR(NOW()), MONTH(NOW());

-- ---------- CONDITIONAL ----------
SELECT name,
       CASE WHEN age < 18 THEN 'minor'
            WHEN age < 65 THEN 'adult'
            ELSE 'senior' END AS category,
       IFNULL(email, 'no email') AS email,
       COALESCE(email, name, 'unknown') AS contact
FROM customers;

-- ---------- INDEXES ----------
CREATE INDEX idx_orders_customer ON orders(customer_id);
CREATE UNIQUE INDEX idx_email ON customers(email);
SHOW INDEX FROM orders;
DROP INDEX idx_orders_customer ON orders;
EXPLAIN SELECT * FROM orders WHERE customer_id = 1;

-- ---------- VIEWS ----------
CREATE VIEW paid_orders AS SELECT * FROM orders WHERE status = 'paid';
SELECT * FROM paid_orders;
DROP VIEW paid_orders;

-- ---------- TRANSACTIONS ----------
START TRANSACTION;
UPDATE customers SET age = age + 1 WHERE id = 1;
INSERT INTO orders (customer_id, total) VALUES (1, 99.90);
COMMIT;        -- or ROLLBACK;

-- ---------- STORED PROCEDURE / FUNCTION / TRIGGER ----------
DELIMITER //
CREATE PROCEDURE get_customer_orders(IN cid INT)
BEGIN
    SELECT * FROM orders WHERE customer_id = cid;
END //

CREATE FUNCTION order_tax(total DECIMAL(10,2)) RETURNS DECIMAL(10,2)
DETERMINISTIC
BEGIN
    RETURN total * 0.17;
END //

CREATE TRIGGER before_order_insert
BEFORE INSERT ON orders
FOR EACH ROW
BEGIN
    IF NEW.order_date IS NULL THEN
        SET NEW.order_date = CURDATE();
    END IF;
END //
DELIMITER ;

CALL get_customer_orders(1);

-- ---------- USERS & PRIVILEGES ----------
CREATE USER 'app'@'localhost' IDENTIFIED BY 'StrongPass123!';
GRANT SELECT, INSERT, UPDATE ON shop.* TO 'app'@'localhost';
REVOKE INSERT ON shop.* FROM 'app'@'localhost';
FLUSH PRIVILEGES;
DROP USER 'app'@'localhost';

-- ---------- BACKUP / RESTORE ----------
-- mysqldump -u root -p shop > shop_backup.sql
-- mysql -u root -p shop < shop_backup.sql
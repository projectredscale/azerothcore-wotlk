--
-- Proxy Protocol v2 reports the address the proxy saw, and that address is IPv6 when the proxy's
-- frontend accepted the connection over IPv6. These four columns were sized for an IPv4 literal,
-- so writing such an address failed with "[1406] Data too long for column ..." and the write was
-- dropped, leaving only "Unhandled MySQL errno 1406. Unexpected behaviour possible." in the log.
ALTER TABLE `account`
    MODIFY COLUMN `last_ip` varchar(45) CHARACTER SET `utf8mb4` COLLATE `utf8mb4_unicode_ci` NOT NULL DEFAULT '127.0.0.1',
    MODIFY COLUMN `last_attempt_ip` varchar(45) CHARACTER SET `utf8mb4` COLLATE `utf8mb4_unicode_ci` NOT NULL DEFAULT '127.0.0.1';

ALTER TABLE `ip_banned`
    MODIFY COLUMN `ip` varchar(45) CHARACTER SET `utf8mb4` COLLATE `utf8mb4_unicode_ci` NOT NULL DEFAULT '127.0.0.1';

ALTER TABLE `logs_ip_actions`
    MODIFY COLUMN `ip` varchar(45) CHARACTER SET `utf8mb4` COLLATE `utf8mb4_unicode_ci` NOT NULL DEFAULT '127.0.0.1';

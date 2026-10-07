--
-- Proxy Protocol v2 reports the address the proxy saw, and that address is IPv6 when the proxy's
-- frontend accepted the connection over IPv6. This column was sized for an IPv4 literal, and a
-- rated arena match writes the whole match, one row per member, in a single transaction: the
-- "[1406] Data too long for column 'ip'" error aborted that transaction and the arena log for the
-- match was lost, not just the one address.
ALTER TABLE `log_arena_memberstats`
    MODIFY COLUMN `ip` varchar(45) CHARACTER SET `utf8mb4` COLLATE `utf8mb4_unicode_ci` NOT NULL;

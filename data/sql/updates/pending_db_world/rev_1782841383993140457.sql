--
-- Fixes Guardian of Blizzard not attacking
UPDATE `creature_template` SET `unit_flags2` = `unit_flags2` | 262144 WHERE (`entry` = 5764);

SELECT parseDateTimeBestEffort('20220101-010203', 'UTC');
SELECT parseDateTimeBestEffort('20220101+010203', 'UTC');
SELECT parseDateTimeBestEffort('20220101 010203', 'UTC');
SELECT parseDateTimeBestEffort('20220101T010203', 'UTC');
SELECT parseDateTimeBestEffort('20220101T01:02', 'UTC');
SELECT parseDateTimeBestEffort('20220101-0102', 'UTC');
SELECT parseDateTimeBestEffort('20220101+0102', 'UTC');
SELECT parseDateTimeBestEffort('20220101-010203-01', 'UTC');
SELECT parseDateTimeBestEffort('20220101-010203+0100', 'UTC');
SELECT parseDateTimeBestEffort('20220101-010203-01:00', 'UTC');

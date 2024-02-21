CREATE TEMPORARY TABLE t_proj (t DateTime, id UInt64, PROJECTION p (SELECT id, t ORDER BY toStartOfDay(t))) ENGINE = MergeTree ORDER BY id;
SHOW CREATE TEMPORARY TABLE t_proj FORMAT TSVRaw;

CREATE TEMPORARY TABLE t_proj2 (a UInt32, b UInt32, PROJECTION p (SELECT a ORDER BY b * 2)) ENGINE = MergeTree ORDER BY a;
SHOW CREATE TEMPORARY TABLE t_proj2 FORMAT TSVRaw;

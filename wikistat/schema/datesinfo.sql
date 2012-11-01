CREATE TABLE datesinfo (
  id INTEGER NOT NULL,
  datestring CHAR(11) NOT NULL,
  caldate DATE NOT NULL,
  calday INTEGER NOT NULL,
  calmonth INTEGER NOT NULL,
  calyear INTEGER NOT NULL,
  dayofweek INTEGER NOT NULL,
  dayhour INTEGER NOT NULL
);

-- ALTER TABLE datesinfo ADD PRIMARY KEY (id);

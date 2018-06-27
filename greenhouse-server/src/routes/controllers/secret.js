'use strict';

const logger = require('../../logger');

module.exports.secret = (req, res) => {
  logger.debug('REST secret');

  const date = new Date();

  res.json({
    year: date.getUTCFullYear(),
    month: date.getUTCMonth() + 1,
    day: date.getUTCDay(),
    hours: date.getUTCHours(),
    minutes: date.getUTCMinutes(),
    seconds: date.getUTCSeconds(),
    offset: date.getTimezoneOffset()
  });
};

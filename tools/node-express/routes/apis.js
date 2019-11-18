const express = require( 'express' );
const router = new express.Router();
const statics = require('./v1/statics')

// routes' entry
router.use('/', statics)

module.exports = router;
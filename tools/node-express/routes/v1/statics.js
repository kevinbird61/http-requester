/** 
 * Routes for static pages
 */
const express = require( 'express' );
const router = new express.Router();


router.use('/software', function(req,res){
    // list current software in classroom
    res.render('software.ejs',{
        title: "已安裝軟體列表"
    })
})

router.use('/about', function(req,res){
    res.render('about.ejs',{
        title: "關於 PC 助教的這檔事 ... "
    })
})

router.use('/', function(req,res){
    res.render('landing.ejs',{
        title: "歡迎使用簡易回報系統！"
    })
})

module.exports = router;
const express = require('express');
const app = express();
const port = 3000;


app.use('/css', express.static(__dirname + '/css'));
app.use('/images', express.static(__dirname + '/images'));

app.get('/', (req, res) => {
    res.sendFile(__dirname + '/index.html');
});

app.listen(port, () => {
  console.log(`Running app on port http://localhost:${port}`);
});
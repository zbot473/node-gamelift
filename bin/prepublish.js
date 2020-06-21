const fs = require('fs')
const path = require('path')

function convertSymlinksToFiles(dirPath) {
  const files = fs.readdirSync(dirPath)
  files.forEach((file) => {
    if (fs.statSync(`${dirPath}/${file}`).isDirectory()) {
      convertSymlinksToFiles(`${dirPath}/${file}`)
    } else {
      const filePath = path.join(`${dirPath}`, '/', file)
      if (fs.lstatSync(filePath).isSymbolicLink()) {
        const origFilePath = path.join(`${dirPath}`, fs.readlinkSync(filePath))
        fs.unlinkSync(filePath)
        fs.copyFileSync(origFilePath, filePath)
      }
    }
  })
}

convertSymlinksToFiles(path.join(__dirname, '/../gamelift/lib'))

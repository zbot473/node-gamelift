#!/usr/bin/env node

const argv = require('yargs')
  .scriptName('node-gamelift-build')
  .command('$0 [args]')
  .option('entry-path', {
    alias: 'e',
    type: 'string',
    description: 'entry path script e.g. index.js',
    nargs: 1,
    demand: true,
  }).argv

const appRoot = require('app-root-path')
const tmp = require('tmp')
const fs = require('fs')
const ncp = require('ncp').ncp
const os = require('os')
const { exec } = require('pkg')
const path = require('path')

async function main(entryPath, buildDir, execName, nodeVersion) {
  console.info('Creating GameLift build...')

  const dir = tmp.dirSync()
  try {
    await mkdir(`${dir.name}/gamelift/lib/${os.platform}-${os.arch}`)
    await copyDir(
      `${appRoot}/node_modules/node-gamelift/gamelift/lib/${os.platform}-${os.arch}`,
      `${dir.name}/gamelift/lib/${os.platform}-${os.arch}`
    )

    await mkdir(`${dir.name}/build/bin`)
    await copyFile(
      `${appRoot}/node_modules/node-gamelift/build/Release/node-gamelift.node`,
      `${dir.name}/build/bin/node-gamelift.node`
    )

    await exec([
      entryPath,
      '--target',
      `${nodeVersion}-${os.platform}-${os.arch}`,
      '--output',
      `${dir.name}/build/bin/${execName}`,
    ])

    await rmdir(buildDir)
    await copyDir(dir.name, buildDir)

    console.info(`Created! Build root: ${path.resolve(`${buildDir}/.`)}`)
  } catch (err) {
    console.error(err)
  }
}

entryPath = argv.entryPath
buildDir = argv.buildDir || 'gamelift-build'
const packageName = JSON.parse(fs.readFileSync(`${appRoot}/package.json`)).name
execName = argv.execName || packageName
nodeVersion = argv.nodeVersion || 'node12'

if (os.platform() !== 'linux' && os.platform() !== 'win32') {
  console.warn(
    'WARNING: GameLift only supports Linux and Windows for server builds'
  )
}

main(entryPath, buildDir, execName, nodeVersion)

async function mkdir(path) {
  return new Promise((resolve, reject) => {
    fs.mkdir(path, { recursive: true }, function (err) {
      if (err) {
        reject(err)
        return
      }
      resolve()
    })
  })
}

async function copyDir(src, dest) {
  return new Promise((resolve, reject) => {
    ncp(src, dest, function (err) {
      if (err) {
        reject(err)
        return
      }
      resolve()
    })
  })
}

async function rmdir(path) {
  return new Promise((resolve, reject) => {
    fs.rmdir(path, { recursive: true }, function (err) {
      if (err) {
        reject(err)
        return
      }
      resolve()
    })
  })
}

async function copyFile(src, dest) {
  return new Promise((resolve, reject) => {
    fs.copyFile(src, dest, function (err) {
      if (err) {
        reject(err)
        return
      }
      resolve()
    })
  })
}

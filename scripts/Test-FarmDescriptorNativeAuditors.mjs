import fs from 'node:fs';
import path from 'node:path';
import crypto from 'node:crypto';
import assert from 'node:assert/strict';
import {inspectFarmDescriptorNative} from './Inspect-FarmDescriptorNative.mjs';
import {inspectFarmDescriptorProject} from './Inspect-FarmDescriptorProject.mjs';

const [directoryArg, destination] = process.argv.slice(2); assert(directoryArg && destination); assert(!fs.existsSync(destination));
const directory = path.resolve(directoryArg), nativePath = path.join(directory, 'FarmDescriptorSaved.spp'), projectPath = path.join(directory, path.basename(directory) + '.pro');
const hash = p => crypto.createHash('sha256').update(fs.readFileSync(p)).digest('hex'), native = fs.readFileSync(nativePath), project = fs.readFileSync(projectPath);
const before = {native: hash(nativePath), project: hash(projectPath)}, controls = [];
assert(inspectFarmDescriptorNative(directory, nativePath, 'FarmDescriptorSaved').passed); assert(inspectFarmDescriptorProject(directory).passed); controls.push({name: 'Unchanged native documents', accepted: true});
function reject(name, buffer, offset, check) {assert(offset >= 8 && offset < buffer.length); const changed = Buffer.from(buffer); changed[offset] ^= 1; let error; try {check(changed);} catch (e) {error = String(e.message);} assert(error, 'Accepted corrupt native evidence: ' + name); controls.push({name, accepted: false, error});}
const checkScript = nativeOverride => inspectFarmDescriptorNative(directory, nativePath, 'FarmDescriptorSaved', {nativeOverride});
reject('Saved Unicode name changed', native, native.indexOf(Buffer.from('FarmDescriptorSaved\0', 'utf16le')), checkScript);
reject('Script source changed', native, native.indexOf(Buffer.from('scsr')) + 8, checkScript);
reject('Script document GUID changed', native, native.indexOf(Buffer.from('guid')) + 8, checkScript);
reject('Container descriptor changed', native, native.indexOf(Buffer.from('refh')) + 24, checkScript);
const checkProject = projectOverride => inspectFarmDescriptorProject(directory, {projectOverride});
reject('Project RIFF form changed', project, 8, checkProject);
reject('Project catalog path changed', project, project.indexOf(Buffer.from('DescriptorHost.sgp\0', 'utf16le')), checkProject);
reject('Project document size changed', project, project.indexOf(Buffer.from('filh')) + 8 + 24, checkProject);
reject('Project document GUID changed', project, project.indexOf(Buffer.from('filh')) + 8 + 28, checkProject);
reject('Project runtime filename changed', project, project.indexOf(Buffer.from('DescriptorHost.sgt\0', 'utf16le')), checkProject);
assert.equal(hash(nativePath), before.native); assert.equal(hash(projectPath), before.project);
const proof = {schema: 1, createdUtc: new Date().toISOString(), passed: true, inputs: before, controls, driver: {path: path.resolve(process.argv[1]), sha256: hash(process.argv[1])}, inspectors: ['scripts/Inspect-FarmDescriptorNative.mjs', 'scripts/Inspect-FarmDescriptorProject.mjs'].map(p => ({path: path.resolve(p), sha256: hash(p)})), fullAcceptance: false};
fs.writeFileSync(destination, JSON.stringify(proof, null, 2) + '\n', {flag: 'wx'}); console.log(JSON.stringify({passed: true, controls: controls.length, fullAcceptance: false}));

fs     = require('fs');
path   = require('path');
mkdirp = require('mkdirp');
util   = require('util');

function logError(err) { if(err) console.log(err); }

function sanitizeKey(key)
{
  return key.toLowerCase().replace(/\s/g, "_");
}

function fprintf(f, format)
{
  var args = Array.prototype.slice.call(arguments);
  args.shift();
  return f.write(util.format(format, args));
}

function main() 
{
  if (process.argv.length < 3) {
    console.log("usage : <input> [output]");
    return;
  }
  
  var spinePath = path.normalize(process.argv[2]);

  fs.readFile(spinePath, 'utf8', function (err,data) {
    if (err) return console.log(err);

    var spineModel = JSON.parse(data);
    var outputName = path.basename(spinePath, ".json");
    var outputPath = process.argv.length == 4 ? 
      path.normalize(path.extname(process.argv[3]) == ".pgi" ? 
        process.argv[3] : 
        process.argv[3] + ".pgi") :
      path.normalize(outputName + ".pgi");

    mkdirp.sync(path.dirname(outputPath));

    var f = fs.createWriteStream(outputPath);
    //-------------------------------------------------------------------------
    // write the pgi nodes
    //-------------------------------------------------------------------------

    fprintf(f, "[nodes]");
    for (var i in spineModel.bones)
    {
      var bone = spineModel.bones[i];
      fprintf(f, "%s: {\n", sanitizeKey(bone.name));
      fprintf(f, "  parent = %s", bone.parent ? util.format("\"%s\"", bone.parent) : "null");
      fprintf(f, "}\n")
    }
  });
}

main();
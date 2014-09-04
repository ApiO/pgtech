// generate data for the sandbox : 
// "node pgi_exporter_b2editor.js plateforms.json path/to/sandbox/resources units default_static default solid"

fs     = require('fs');
PNG    = require('pngjs').PNG;
path   = require('path');
mkdirp = require('mkdirp');
util   = require('util');

function create_atlas(name) {
  var atlas = fs.createWriteStream(name);
  atlas.write('{\n\t"type": "ATLAS",\n\t"regions": {');
  return atlas;
}

function close_atlas(atlas) {
  atlas.end('\n\t}\n}');
}

function create_resources(config, body, atlas, last) {
  var p = path.resolve(path.dirname(config.input), body.imagePath);
  var f = fs.createReadStream(p)
    .pipe(new PNG({filterType: 4}))
    .on('parsed', function() {
      var actor = { "template": config.actor_template, "shapes": {} };
      var shape_translation = [ 
        -1 * this.width/2  + body.origin.x, 
        -1 * this.height/2 + body.origin.y];
      // create shapes
      for(var i in body.polygons) {
        var poly = body.polygons[i];
        var shape_name = util.format("shape_%d", i);
        var shape_file = util.format("%s_%s", body.name, i);

        actor.shapes[shape_name] = { 
          "template" : config.shape_template, 
          "material" : config.material,
          "shape"    : config.rel_output + "/" + shape_file,
          "translation" : shape_translation
        };

        var shape = {
          "type"  : "POLYGON",
          "value" : []
        };

        for (var j in poly) {
          shape.value.push(Math.round(poly[j].x * this.width));
          shape.value.push(Math.round(poly[j].y * this.width));
        }
        fs.createWriteStream(config.abs_output + "/" + shape_file + ".pgshape")
          .write(JSON.stringify(shape, null, 2));
      }

      // write the actor
      fs.createWriteStream(util.format("%s/%s.pgactor", config.abs_output, body.name))
        .write(JSON.stringify(actor, null, 2));

      // copy the image into the export folder
      var img_file = util.format("textures/%s.png", body.name);
      this.pack().pipe(fs.createWriteStream(config.abs_output + "/" + img_file));

      // write the atlas
      atlas.write(util.format('\n\t\t"%s": { "file": "%s/%s" }', body.name, config.rel_output, img_file));
      if (last) 
        close_atlas(atlas);
      else
        atlas.write(',');

      // create the sprite
      var sprite = {
        "setup_frame" : "default",
        "frames" : {
          "default" : {
            "texture" : config.rel_output + "/atlas#" + body.name
          }
        }
      };
      
      fs.createWriteStream(util.format("%s/%s.pgsprite", config.abs_output, body.name))
        .write(JSON.stringify(sprite, null, 2));

      // create the unit
      var unit = {
        "nodes" : { "root" : {} },
        "sprites" : {
          "main" : {
            "node" : "root",
            "template" : config.rel_output + "/" + body.name,
            "order" : 0
          }
        },
        "actors" : {
          "main" : {
            "node" : "root",
            "actor" : config.rel_output + "/" + body.name
          }
        }
      };

      fs.createWriteStream(util.format("%s/%s.pgunit", config.abs_output, body.name))
        .write(JSON.stringify(unit, null, 2));
    });
}

function main()  {

  if (process.argv.length < 4) {
    console.log("usage : <input> <project_root> [output] [actor_template] [shape_template] [material]");
    return;
  }

  var config = {
    "input"          : path.normalize(process.argv[2]),
    "rel_output"     : process.argv.length >= 5 ? (process.argv[4] + "/") : "",
    "abs_output"     : process.argv[3] + "/",
    "actor_template" : process.argv.length >= 6 ? process.argv[5] : "default",
    "shape_template" : process.argv.length >= 7 ? process.argv[6] : "default",
    "material"       : process.argv.length >= 8 ? process.argv[7] : "default"
  };
  config.rel_output += path.basename(config.input, ".json")
  config.abs_output += config.rel_output;

  fs.readFile(config.input, 'utf8', function (err,data) {
    if (err) return console.log(err);
    var project = JSON.parse(data);
    mkdirp.sync(config.abs_output);
    mkdirp.sync(config.abs_output + "/textures");
    var atlas = create_atlas(config.abs_output + "/atlas.pgtex");

    for(var i in project.rigidBodies) {
      var body = project.rigidBodies[i];
      create_resources(config, body, atlas, i == project.rigidBodies.length - 1);
    }
  });
}

main();



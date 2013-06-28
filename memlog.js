
// Analyze the output of memlog.so and print out leaked memory blocks.

var fs = require('fs');

var log = fs.readFileSync(process.argv[2], 'ascii').split('\n');
var mem = {};

for (var i = 0; i < log.length; ++i) {
  var line  = log[i].split(' ');
  var trace = [];

  if ((line[0] == 'm') || (line[0] == 'r') || (line[0] == 'f')) {
    while (log[++i] != '') {
      trace.push(log[i]);
    }
  } else {
    continue;
  }

  switch (line[0]) {
    case 'r': {
      delete mem[line[2]];
    }

    case 'm': {
      mem[line[1]] = trace.join('\n');
      break;
    }

    case 'f': {
      delete mem[line[1]];
      break;
    }
  }
}


for (var p in mem) {
  console.log(p + '\n' + mem[p] + '\n\n');
}


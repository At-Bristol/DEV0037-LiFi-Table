const createSurfaceMap = (width = 98, length = 36, type = 'alternatingRows') => {

  let map = [];

  const createMapOfType = () => {
    var map = new Uint16Array(width*length);
    for(var i = 0; i<map.length; i++) {
        var row = Math.floor(i/width), col = i % width;

        if((row % 2) === 0) {
            map[i] = i;
        } else {
            map[i] = (row+1) * width - (col+1);
        }
    }

    return map;
  }

  const init = () => {
    try{
      map = createMapOfType()
    } catch (e) {
      console.error(e)
    }

  }
  init()

  const apply = arr => arr.map((e,i) => arr[map[i]])

  return {
    init,
    apply
  }

}

module.exports = createSurfaceMap

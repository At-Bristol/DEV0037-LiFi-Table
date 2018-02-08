import dat from 'dat.gui';
const gui = new dat.GUI();

const text = {
  message: 'yo',
  speed: 0,
}

gui.add(text, 'message');
gui.add(text, 'speed', -5, 5);


export default gui
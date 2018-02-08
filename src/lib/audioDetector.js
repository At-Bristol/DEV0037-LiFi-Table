var audioContext = new AudioContext();
var analyserContext;
var analyserNode;
var fftSize = simResolution * 2;

export const initAudio = function () {

  console.log("initAudio");

  if (!navigator.getUserMedia)
    navigator.getUserMedia = navigator.webkitGetUserMedia || navigator.mozGetUserMedia;
  if (!navigator.cancelAnimationFrame)
    navigator.cancelAnimationFrame = navigator.webkitCancelAnimationFrame || navigator.mozCancelAnimationFrame;
  if (!navigator.requestAnimationFrame)
    navigator.requestAnimationFrame = navigator.webkitRequestAnimationFrame || navigator.mozRequestAnimationFrame;

  navigator.getUserMedia({
    "audio": {
      "mandatory": {
        "googEchoCancellation": "false",
        "googAutoGainControl": "false",
        "googNoiseSuppression": "false",
        "googHighpassFilter": "false"
      },
      "optional": []
    },
  }, audioAnalyseInit, function (e) {
    alert('Error getting audio');
    console.log(e);
  });
};

function audioAnalyseInit(stream) {
  console.log('gotStream')

  inputPoint = audioContext.createGain();

  // Create an AudioNode from the stream.
  realAudioInput = audioContext.createMediaStreamSource(stream);
  audioInput = realAudioInput;
  audioInput.connect(inputPoint);

  //audioInput = convertToMono( input );

  analyserNode = audioContext.createAnalyser();
  analyserNode.fftSize = simResolution * 2;
  inputPoint.connect(analyserNode);

  //audioRecorder = new Recorder( inputPoint );

  zeroGain = audioContext.createGain();
  zeroGain.gain.value = 0.0;
  inputPoint.connect(zeroGain);
  zeroGain.connect(audioContext.destination);
  init3d()
};


function updateAnalysers() {

  var freqByteData = new Uint8Array(analyserNode.frequencyBinCount);
  analyserNode.getByteFrequencyData(freqByteData);
  return (freqByteData);

};


function bassHit() {
  //bassHit = Math.max(extractFreqRange(100,500,freqByteData)-20,0);
};

function extractFreqRange(start, end, byteData) {
  var sampleRate = 44100;
  var hzBandRange = sampleRate / (fftSize / 2)
  var value = 0
  var numberOfValues = 0
  i = 0
  while (i * hzBandRange <= end) {
    if (i * hzBandRange >= start) {
      value += byteData[i]
      numberOfValues++
    }
    i++
  }
  return value / numberOfValues;
}

export default initAudio;


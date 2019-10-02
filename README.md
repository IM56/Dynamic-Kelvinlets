# Dynamic-Kelvinlets
Simple app for creating procedural elastic animations, written in DirectX 11 and C++

<h2>Getting started</h2>
<p>To use this application, you can build the project from source. Please ensure that the Framework static library is built first 
and linked to the Kelvinlets project. If you are using Visual Studio, please add Framework to the 'Additional Include Directories' 
so that the application has access to the necessary header files.</p>

<h2>What does it do?</h2>
<p>Using the theory laid out in the papers <a href="https://graphics.pixar.com/library/Kelvinlets/paper.pdf">Regularized Kelvinlets: Sculpting Brushes based on Fundamental Solutions of Elasticity</a> and <a href="https://graphics.pixar.com/library/DynaKelvinlets/paper.pdf">Dynamic Kelvinlets: Secondary Motions based on Fundamental Solutions of Elastodynamics</a> by Fernando de Goes, Doug L. James, I implemented a means of applying elastic deformations to any otherwise static mesh.</p>

<p>Elastic deformations are decomposed into <b>point impulses</b>, <b>pinches</b> and <b>scaling transformations</b>. The application allows you to add or remove multiple Kelvinlets to a mesh in any order, and play the resulting animations as a timeline. The user is able to control the playback speed of the animations and can use the slider bar to jump to any point in the timeline.</p>

<p>To see the application in action, please watch the <a href="https://github.com/IM56/Dynamic-Kelvinlets/tree/master/Videos">demo videos</a>.
  For more information about the application's design and implementation, please check out the <a href="https://github.com/IM56/Dynamic-Kelvinlets/blob/master/Technical%20Article%20-%20Dynamic%20Kelvinlets.pdf">accompanying report</a>.</p>

<h2>Camera controls</h2>
<p>The user can move the camera's line of sight by holding right-click and moving the mouse. Whilst right-click is held down, the user can also strafe left (A key), strafe right (D key) and zoom in (W key) and zoom out (S key).</p>

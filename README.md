# Dynamic-Kelvinlets
Simple app for creating procedural elastic animations, written in DirectX 11 and C++

<h2>Getting started</h2>
<p>To use this application, you can build the project from source. Please ensure that the Framework static library is built first 
and linked to the Kelvinlets project. If you are using Visual Studio, please add Framework to the 'Additional Include Directories' 
so that the application has access to the necessary header files.</p>

<h2>What does it do?</h2>
<p>Using the theory laid out in the papers [Regularized Kelvinlets: Sculpting Brushes based on Fundamental Solutions of Elasticity](https://graphics.pixar.com/library/Kelvinlets/paper.pdf) and [Dynamic Kelvinlets: Secondary Motions based on Fundamental Solutions of Elastodynamics](https://graphics.pixar.com/library/DynaKelvinlets/paper.pdf) by Fernando de Goes, Doug L. James, I implemented a means of applying elastic deformations to any otherwise static mesh.</p>

<p>Elastic deformations are decomposed into <b>point impulses</b>, <b>pinches</b> and <b>scaling transformations</b>. The application allows you to add or remove multiple Kelvinlets to a mesh in any order, and play the resulting animations as a timeline. The user is able to control the playback speed of the animations and can use the slider bar to jump to any point in the timeline.</p>

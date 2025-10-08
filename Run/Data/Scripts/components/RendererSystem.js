//----------------------------------------------------------------------------------------------------
// RendererSystem.js - Renderer subsystem wrapper
//----------------------------------------------------------------------------------------------------
import { Subsystem } from '../core/Subsystem.js';

/**
 * RendererSystem - Wrapper for the global renderer interface
 *
 * Provides rendering utilities and manages the renderer state.
 * Priority: 100 (renders last, after all updates)
 */
export class RendererSystem extends Subsystem {
    constructor() {
        super('rendererSystem', 100, {enabled: true}); // Render last

        this.data = {
            initialized: false
        };
    }

    /**
     * Update - Initialize renderer on first frame
     */
    update(gameDelta, systemDelta) {
        if (!this.data.initialized) {
            this.initialize();
            this.data.initialized = true;
        }
    }

    /**
     * Initialize renderer
     */
    initialize() {
        if (typeof renderer === 'undefined') {
            console.error('RendererSystem: renderer global object not available!');
            return;
        }
    }

    /**
     * Helper: Create a cube vertex array
     * Returns vertex array handle
     */
    createCubeVertexArray(size = 1.0) {
        const handle = renderer.createVertexArrayCPP();

        const half = size / 2;

        // Define 8 corners of a cube
        const corners = [
            { x: -half, y: -half, z: -half }, // 0: FBL
            { x:  half, y: -half, z: -half }, // 1: FBR
            { x:  half, y: -half, z:  half }, // 2: FTR
            { x: -half, y: -half, z:  half }, // 3: FTL
            { x: -half, y:  half, z: -half }, // 4: BBL
            { x:  half, y:  half, z: -half }, // 5: BBR
            { x:  half, y:  half, z:  half }, // 6: BTR
            { x: -half, y:  half, z:  half }  // 7: BTL
        ];

        // Define 6 faces (2 triangles each = 12 triangles = 36 vertices)
        const faces = [
            // Front face (Y-)
            [0, 1, 2, 0, 2, 3],
            // Back face (Y+)
            [5, 4, 7, 5, 7, 6],
            // Left face (X-)
            [4, 0, 3, 4, 3, 7],
            // Right face (X+)
            [1, 5, 6, 1, 6, 2],
            // Top face (Z+)
            [3, 2, 6, 3, 6, 7],
            // Bottom face (Z-)
            [4, 5, 1, 4, 1, 0]
        ];

        // Add all vertices
        faces.forEach((face, faceIndex) => {
            const colors = [
                [255, 0, 0],     // Front: Red
                [0, 255, 255],   // Back: Cyan
                [0, 255, 0],     // Left: Green
                [255, 0, 255],   // Right: Magenta
                [0, 0, 255],     // Top: Blue
                [255, 255, 0]    // Bottom: Yellow
            ];

            face.forEach((cornerIndex, vertIndex) => {
                const corner = corners[cornerIndex];
                const color = colors[faceIndex];

                // UV coordinates (simple mapping)
                const u = (vertIndex % 3) === 0 ? 0 : (vertIndex % 3) === 1 ? 1 : 0.5;
                const v = vertIndex < 3 ? 0 : 1;

                renderer.addVertex(
                    corner.x, corner.y, corner.z,  // position
                    color[0], color[1], color[2], 255,  // color + alpha
                    u, v  // UV
                );
            });
        });

        return handle;
    }

    /**
     * Helper: Create a sphere vertex array
     * Returns vertex array handle
     * Matches C++ Prop::InitializeLocalVertsForSphere()
     */
    createSphereVertexArray(radius = 0.5, numSlices = 32, numStacks = 16) {
        const handle = renderer.createVertexArrayCPP();
        console.log('RendererSystem: Created vertex array handle =', handle);

        // UV mapping
        const uvMinX = 0.0, uvMinY = 0.0;
        const uvMaxX = 1.0, uvMaxY = 1.0;
        const uvWidth = uvMaxX - uvMinX;
        const uvHeight = uvMaxY - uvMinY;

        const center = { x: 0, y: 0, z: 0 }; // Local space, center at origin
        const color = [255, 255, 255]; // White

        // Generate sphere using latitude-longitude method
        for (let stackIndex = 0; stackIndex < numStacks; stackIndex++) {
            const phi1 = Math.PI * (stackIndex / numStacks);
            const phi2 = Math.PI * ((stackIndex + 1) / numStacks);
            const v1 = stackIndex / numStacks;
            const v2 = (stackIndex + 1) / numStacks;

            for (let sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
                const theta1 = 2.0 * Math.PI * (sliceIndex / numSlices);
                const theta2 = 2.0 * Math.PI * ((sliceIndex + 1) / numSlices);
                const u1 = sliceIndex / numSlices;
                const u2 = (sliceIndex + 1) / numSlices;

                // Calculate quad vertices
                const bottomLeft = {
                    x: center.x + radius * Math.sin(phi1) * Math.cos(theta1),
                    y: center.y + radius * Math.sin(phi1) * Math.sin(theta1),
                    z: center.z + radius * Math.cos(phi1)
                };

                const bottomRight = {
                    x: center.x + radius * Math.sin(phi1) * Math.cos(theta2),
                    y: center.y + radius * Math.sin(phi1) * Math.sin(theta2),
                    z: center.z + radius * Math.cos(phi1)
                };

                const topRight = {
                    x: center.x + radius * Math.sin(phi2) * Math.cos(theta2),
                    y: center.y + radius * Math.sin(phi2) * Math.sin(theta2),
                    z: center.z + radius * Math.cos(phi2)
                };

                const topLeft = {
                    x: center.x + radius * Math.sin(phi2) * Math.cos(theta1),
                    y: center.y + radius * Math.sin(phi2) * Math.sin(theta1),
                    z: center.z + radius * Math.cos(phi2)
                };

                // Calculate UV coordinates for this quad
                const uvBL = { u: uvMinX + uvWidth * u1, v: uvMinY + uvHeight * v1 };
                const uvBR = { u: uvMinX + uvWidth * u2, v: uvMinY + uvHeight * v1 };
                const uvTR = { u: uvMinX + uvWidth * u2, v: uvMinY + uvHeight * v2 };
                const uvTL = { u: uvMinX + uvWidth * u1, v: uvMinY + uvHeight * v2 };

                // Add quad as two triangles (6 vertices)
                // Triangle 1: BL, BR, TL
                renderer.addVertex(bottomLeft.x, bottomLeft.y, bottomLeft.z, color[0], color[1], color[2], 255, uvBL.u, uvBL.v);
                renderer.addVertex(bottomRight.x, bottomRight.y, bottomRight.z, color[0], color[1], color[2], 255, uvBR.u, uvBR.v);
                renderer.addVertex(topLeft.x, topLeft.y, topLeft.z, color[0], color[1], color[2], 255, uvTL.u, uvTL.v);

                // Triangle 2: BR, TR, TL
                renderer.addVertex(bottomRight.x, bottomRight.y, bottomRight.z, color[0], color[1], color[2], 255, uvBR.u, uvBR.v);
                renderer.addVertex(topRight.x, topRight.y, topRight.z, color[0], color[1], color[2], 255, uvTR.u, uvTR.v);
                renderer.addVertex(topLeft.x, topLeft.y, topLeft.z, color[0], color[1], color[2], 255, uvTL.u, uvTL.v);
            }
        }

        const totalVertices = numSlices * numStacks * 6;
        console.log(`RendererSystem: Added ${totalVertices} vertices for sphere`);
        console.log('=== createSphereVertexArray END ===');
        return handle;
    }

    /**
     * Helper: Create a grid vertex array
     * Returns vertex array handle
     * Matches C++ Prop::InitializeLocalVertsForGrid()
     */
    createGridVertexArray(gridLineLength = 100.0) {
        console.log('=== createGridVertexArray START ===');
        console.log(`RendererSystem: Creating grid with lineLength=${gridLineLength}`);

        const handle = renderer.createVertexArrayCPP();
        console.log('RendererSystem: Created vertex array handle =', handle);

        const halfLength = gridLineLength / 2.0;

        // Helper function to add box vertices (matching AddVertsForAABB3D)
        const addBoxVertices = (minX, minY, minZ, maxX, maxY, maxZ, colorR, colorG, colorB) => {
            // Define 8 corners
            const fbl = { x: maxX, y: minY, z: minZ };
            const fbr = { x: maxX, y: maxY, z: minZ };
            const ftl = { x: maxX, y: minY, z: maxZ };
            const ftr = { x: maxX, y: maxY, z: maxZ };
            const bbl = { x: minX, y: maxY, z: minZ };
            const bbr = { x: minX, y: minY, z: minZ };
            const btl = { x: minX, y: maxY, z: maxZ };
            const btr = { x: minX, y: minY, z: maxZ };

            // Helper to add quad (6 vertices = 2 triangles)
            const addQuad = (bl, br, tl, tr) => {
                // Triangle 1: BL, BR, TL
                renderer.addVertex(bl.x, bl.y, bl.z, colorR, colorG, colorB, 255, 0, 0);
                renderer.addVertex(br.x, br.y, br.z, colorR, colorG, colorB, 255, 1, 0);
                renderer.addVertex(tl.x, tl.y, tl.z, colorR, colorG, colorB, 255, 0, 1);
                // Triangle 2: BR, TR, TL
                renderer.addVertex(br.x, br.y, br.z, colorR, colorG, colorB, 255, 1, 0);
                renderer.addVertex(tr.x, tr.y, tr.z, colorR, colorG, colorB, 255, 1, 1);
                renderer.addVertex(tl.x, tl.y, tl.z, colorR, colorG, colorB, 255, 0, 1);
            };

            // Add 6 faces
            addQuad(fbl, fbr, ftl, ftr); // Front
            addQuad(bbl, bbr, btl, btr); // Back
            addQuad(bbr, fbl, btr, ftl); // Left
            addQuad(fbr, bbl, ftr, btl); // Right
            addQuad(ftl, ftr, btr, btl); // Top
            addQuad(bbr, bbl, fbl, fbr); // Bottom
        };

        // Generate grid lines
        for (let i = -Math.floor(halfLength); i < Math.floor(halfLength); i++) {
            let lineWidth = 0.05;
            if (i === 0) lineWidth = 0.3; // Thicker center lines

            const halfWidth = lineWidth / 2.0;

            // X-axis line (along X direction)
            const boundsX = {
                minX: -halfLength,
                minY: -halfWidth + i,
                minZ: -halfWidth,
                maxX: halfLength,
                maxY: halfWidth + i,
                maxZ: halfWidth
            };

            // Y-axis line (along Y direction)
            const boundsY = {
                minX: -halfWidth + i,
                minY: -halfLength,
                minZ: -halfWidth,
                maxX: halfWidth + i,
                maxY: halfLength,
                maxZ: halfWidth
            };

            // Color logic: every 5th line is colored, others are dark grey
            let colorX = [64, 64, 64]; // DARK_GREY
            let colorY = [64, 64, 64]; // DARK_GREY

            if (i % 5 === 0) {
                colorX = [255, 0, 0];   // RED
                colorY = [0, 255, 0];   // GREEN
            }

            // Add both lines
            addBoxVertices(boundsX.minX, boundsX.minY, boundsX.minZ, boundsX.maxX, boundsX.maxY, boundsX.maxZ, colorX[0], colorX[1], colorX[2]);
            addBoxVertices(boundsY.minX, boundsY.minY, boundsY.minZ, boundsY.maxX, boundsY.maxY, boundsY.maxZ, colorY[0], colorY[1], colorY[2]);
        }

        const numLines = Math.floor(gridLineLength);
        const verticesPerBox = 36; // 6 faces * 6 vertices per face
        const totalVertices = numLines * 2 * verticesPerBox;
        console.log(`RendererSystem: Added ${totalVertices} vertices for grid (${numLines} lines)`);
        console.log('=== createGridVertexArray END ===');
        return handle;
    }

    /**
     * Helper: Set default rendering state
     */
    setDefaultRenderState() {
        renderer.setBlendMode("OPAQUE");
        renderer.setRasterizerMode("SOLID_CULL_BACK");
        renderer.setSamplerMode("POINT_CLAMP");
        renderer.setDepthMode("READ_WRITE_LESS_EQUAL");
        renderer.bindTextureCPP("null");
        renderer.bindShader("Data/Shaders/Default");
    }
}

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

        const colors = [
            [255, 0, 0],     // Front: Red
            [0, 255, 255],   // Back: Cyan
            [0, 255, 0],     // Left: Green
            [255, 0, 255],   // Right: Magenta
            [0, 0, 255],     // Top: Blue
            [255, 255, 0]    // Bottom: Yellow
        ];

        // Build vertex array in JavaScript (36 vertices * 9 values = 324 elements)
        const vertexArray = [];
        faces.forEach((face, faceIndex) => {
            const color = colors[faceIndex];

            face.forEach((cornerIndex, vertIndex) => {
                const corner = corners[cornerIndex];

                // UV coordinates (simple mapping)
                const u = (vertIndex % 3) === 0 ? 0 : (vertIndex % 3) === 1 ? 1 : 0.5;
                const v = vertIndex < 3 ? 0 : 1;

                // Add vertex data: x, y, z, r, g, b, a, u, v
                vertexArray.push(
                    corner.x, corner.y, corner.z,      // position
                    color[0], color[1], color[2], 255, // color + alpha
                    u, v                                // UV
                );
            });
        });

        // Submit all vertices in single call (36x speedup)
        renderer.addVertexBatch(vertexArray);

        return handle;
    }

    /**
     * Helper: Create a sphere vertex array
     * Returns vertex array handle
     * Matches C++ Prop::InitializeLocalVertsForSphere()
     */
    createSphereVertexArray(radius = 0.5, numSlices = 32, numStacks = 16) {
        console.log('=== createSphereVertexArray START ===');
        console.log(`RendererSystem: Creating sphere with radius=${radius}, slices=${numSlices}, stacks=${numStacks}`);

        const handle = renderer.createVertexArrayCPP();
        console.log('RendererSystem: Created vertex array handle =', handle);

        // UV mapping
        const uvMinX = 0.0, uvMinY = 0.0;
        const uvMaxX = 1.0, uvMaxY = 1.0;
        const uvWidth = uvMaxX - uvMinX;
        const uvHeight = uvMaxY - uvMinY;

        const centerX = 0, centerY = 0, centerZ = 0; // Local space, center at origin
        const r = 255, g = 255, b = 255, a = 255; // White

        // Pre-allocate array for performance: 6 vertices per quad * 9 values per vertex
        const totalValues = numSlices * numStacks * 6 * 9;
        const vertexArray = new Array(totalValues);
        console.log(`RendererSystem: Pre-allocated array with ${totalValues} values`);
        let idx = 0;

        console.log('RendererSystem: Starting vertex generation loop...');
        // Generate sphere using latitude-longitude method
        for (let stackIndex = 0; stackIndex < numStacks; stackIndex++) {
            const phi1 = Math.PI * (stackIndex / numStacks);
            const phi2 = Math.PI * ((stackIndex + 1) / numStacks);
            const v1 = stackIndex / numStacks;
            const v2 = (stackIndex + 1) / numStacks;

            const sinPhi1 = Math.sin(phi1);
            const cosPhi1 = Math.cos(phi1);
            const sinPhi2 = Math.sin(phi2);
            const cosPhi2 = Math.cos(phi2);

            for (let sliceIndex = 0; sliceIndex < numSlices; sliceIndex++) {
                const theta1 = 2.0 * Math.PI * (sliceIndex / numSlices);
                const theta2 = 2.0 * Math.PI * ((sliceIndex + 1) / numSlices);
                const u1 = sliceIndex / numSlices;
                const u2 = (sliceIndex + 1) / numSlices;

                const cosTheta1 = Math.cos(theta1);
                const sinTheta1 = Math.sin(theta1);
                const cosTheta2 = Math.cos(theta2);
                const sinTheta2 = Math.sin(theta2);

                // Calculate quad vertices (avoid object allocations)
                const blX = centerX + radius * sinPhi1 * cosTheta1;
                const blY = centerY + radius * sinPhi1 * sinTheta1;
                const blZ = centerZ + radius * cosPhi1;

                const brX = centerX + radius * sinPhi1 * cosTheta2;
                const brY = centerY + radius * sinPhi1 * sinTheta2;
                const brZ = centerZ + radius * cosPhi1;

                const trX = centerX + radius * sinPhi2 * cosTheta2;
                const trY = centerY + radius * sinPhi2 * sinTheta2;
                const trZ = centerZ + radius * cosPhi2;

                const tlX = centerX + radius * sinPhi2 * cosTheta1;
                const tlY = centerY + radius * sinPhi2 * sinTheta1;
                const tlZ = centerZ + radius * cosPhi2;

                // Calculate UV coordinates (avoid object allocations)
                const uvBLu = uvMinX + uvWidth * u1;
                const uvBLv = uvMinY + uvHeight * v1;
                const uvBRu = uvMinX + uvWidth * u2;
                const uvBRv = uvMinY + uvHeight * v1;
                const uvTRu = uvMinX + uvWidth * u2;
                const uvTRv = uvMinY + uvHeight * v2;
                const uvTLu = uvMinX + uvWidth * u1;
                const uvTLv = uvMinY + uvHeight * v2;

                // Triangle 1: BL, BR, TL (9 values * 3 vertices = 27 values)
                vertexArray[idx++] = blX; vertexArray[idx++] = blY; vertexArray[idx++] = blZ;
                vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = a;
                vertexArray[idx++] = uvBLu; vertexArray[idx++] = uvBLv;

                vertexArray[idx++] = brX; vertexArray[idx++] = brY; vertexArray[idx++] = brZ;
                vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = a;
                vertexArray[idx++] = uvBRu; vertexArray[idx++] = uvBRv;

                vertexArray[idx++] = tlX; vertexArray[idx++] = tlY; vertexArray[idx++] = tlZ;
                vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = a;
                vertexArray[idx++] = uvTLu; vertexArray[idx++] = uvTLv;

                // Triangle 2: BR, TR, TL (9 values * 3 vertices = 27 values)
                vertexArray[idx++] = brX; vertexArray[idx++] = brY; vertexArray[idx++] = brZ;
                vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = a;
                vertexArray[idx++] = uvBRu; vertexArray[idx++] = uvBRv;

                vertexArray[idx++] = trX; vertexArray[idx++] = trY; vertexArray[idx++] = trZ;
                vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = a;
                vertexArray[idx++] = uvTRu; vertexArray[idx++] = uvTRv;

                vertexArray[idx++] = tlX; vertexArray[idx++] = tlY; vertexArray[idx++] = tlZ;
                vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = a;
                vertexArray[idx++] = uvTLu; vertexArray[idx++] = uvTLv;
            }
        }
        console.log('RendererSystem: Vertex generation complete, submitting to C++...');


        // Submit all vertices in single call (batch API)
        renderer.addVertexBatch(vertexArray);

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
        const halfLength = gridLineLength / 2.0;
        console.log('RendererSystem: Created vertex array handle =', handle);
        // Pre-calculate array size: 100 lines × 2 (X and Y) × 36 vertices/box × 9 values/vertex
        const numLines = Math.floor(gridLineLength);
        const verticesPerBox = 36;
        const valuesPerVertex = 9;
        const totalValues = numLines * 2 * verticesPerBox * valuesPerVertex;

        // Pre-allocate array for better performance
        const vertexArray = new Array(totalValues);
        let idx = 0;

        // Optimized inline box generation (no object allocations)
        const addBox = (minX, minY, minZ, maxX, maxY, maxZ, r, g, b) => {
            // Define corners inline (no object allocations)
            // Front face corners
            const fbl_x = maxX, fbl_y = minY, fbl_z = minZ;
            const fbr_x = maxX, fbr_y = maxY, fbr_z = minZ;
            const ftl_x = maxX, ftl_y = minY, ftl_z = maxZ;
            const ftr_x = maxX, ftr_y = maxY, ftr_z = maxZ;
            // Back face corners
            const bbl_x = minX, bbl_y = maxY, bbl_z = minZ;
            const bbr_x = minX, bbr_y = minY, bbr_z = minZ;
            const btl_x = minX, btl_y = maxY, btl_z = maxZ;
            const btr_x = minX, btr_y = minY, btr_z = maxZ;

            // Front face (2 triangles = 6 vertices × 9 values = 54 values)
            vertexArray[idx++] = fbl_x; vertexArray[idx++] = fbl_y; vertexArray[idx++] = fbl_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 0; vertexArray[idx++] = 0;

            vertexArray[idx++] = fbr_x; vertexArray[idx++] = fbr_y; vertexArray[idx++] = fbr_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 1; vertexArray[idx++] = 0;

            vertexArray[idx++] = ftl_x; vertexArray[idx++] = ftl_y; vertexArray[idx++] = ftl_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 0; vertexArray[idx++] = 1;

            vertexArray[idx++] = fbr_x; vertexArray[idx++] = fbr_y; vertexArray[idx++] = fbr_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 1; vertexArray[idx++] = 0;

            vertexArray[idx++] = ftr_x; vertexArray[idx++] = ftr_y; vertexArray[idx++] = ftr_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 1; vertexArray[idx++] = 1;

            vertexArray[idx++] = ftl_x; vertexArray[idx++] = ftl_y; vertexArray[idx++] = ftl_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 0; vertexArray[idx++] = 1;

            // Back face (54 values)
            vertexArray[idx++] = bbl_x; vertexArray[idx++] = bbl_y; vertexArray[idx++] = bbl_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 0; vertexArray[idx++] = 0;

            vertexArray[idx++] = bbr_x; vertexArray[idx++] = bbr_y; vertexArray[idx++] = bbr_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 1; vertexArray[idx++] = 0;

            vertexArray[idx++] = btl_x; vertexArray[idx++] = btl_y; vertexArray[idx++] = btl_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 0; vertexArray[idx++] = 1;

            vertexArray[idx++] = bbr_x; vertexArray[idx++] = bbr_y; vertexArray[idx++] = bbr_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 1; vertexArray[idx++] = 0;

            vertexArray[idx++] = btr_x; vertexArray[idx++] = btr_y; vertexArray[idx++] = btr_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 1; vertexArray[idx++] = 1;

            vertexArray[idx++] = btl_x; vertexArray[idx++] = btl_y; vertexArray[idx++] = btl_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 0; vertexArray[idx++] = 1;

            // Left face (54 values)
            vertexArray[idx++] = bbr_x; vertexArray[idx++] = bbr_y; vertexArray[idx++] = bbr_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 0; vertexArray[idx++] = 0;

            vertexArray[idx++] = fbl_x; vertexArray[idx++] = fbl_y; vertexArray[idx++] = fbl_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 1; vertexArray[idx++] = 0;

            vertexArray[idx++] = btr_x; vertexArray[idx++] = btr_y; vertexArray[idx++] = btr_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 0; vertexArray[idx++] = 1;

            vertexArray[idx++] = fbl_x; vertexArray[idx++] = fbl_y; vertexArray[idx++] = fbl_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 1; vertexArray[idx++] = 0;

            vertexArray[idx++] = ftl_x; vertexArray[idx++] = ftl_y; vertexArray[idx++] = ftl_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 1; vertexArray[idx++] = 1;

            vertexArray[idx++] = btr_x; vertexArray[idx++] = btr_y; vertexArray[idx++] = btr_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 0; vertexArray[idx++] = 1;

            // Right face (54 values)
            vertexArray[idx++] = fbr_x; vertexArray[idx++] = fbr_y; vertexArray[idx++] = fbr_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 0; vertexArray[idx++] = 0;

            vertexArray[idx++] = bbl_x; vertexArray[idx++] = bbl_y; vertexArray[idx++] = bbl_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 1; vertexArray[idx++] = 0;

            vertexArray[idx++] = ftr_x; vertexArray[idx++] = ftr_y; vertexArray[idx++] = ftr_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 0; vertexArray[idx++] = 1;

            vertexArray[idx++] = bbl_x; vertexArray[idx++] = bbl_y; vertexArray[idx++] = bbl_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 1; vertexArray[idx++] = 0;

            vertexArray[idx++] = btl_x; vertexArray[idx++] = btl_y; vertexArray[idx++] = btl_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 1; vertexArray[idx++] = 1;

            vertexArray[idx++] = ftr_x; vertexArray[idx++] = ftr_y; vertexArray[idx++] = ftr_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 0; vertexArray[idx++] = 1;

            // Top face (54 values)
            vertexArray[idx++] = ftl_x; vertexArray[idx++] = ftl_y; vertexArray[idx++] = ftl_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 0; vertexArray[idx++] = 0;

            vertexArray[idx++] = ftr_x; vertexArray[idx++] = ftr_y; vertexArray[idx++] = ftr_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 1; vertexArray[idx++] = 0;

            vertexArray[idx++] = btr_x; vertexArray[idx++] = btr_y; vertexArray[idx++] = btr_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 0; vertexArray[idx++] = 1;

            vertexArray[idx++] = ftr_x; vertexArray[idx++] = ftr_y; vertexArray[idx++] = ftr_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 1; vertexArray[idx++] = 0;

            vertexArray[idx++] = btl_x; vertexArray[idx++] = btl_y; vertexArray[idx++] = btl_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 1; vertexArray[idx++] = 1;

            vertexArray[idx++] = btr_x; vertexArray[idx++] = btr_y; vertexArray[idx++] = btr_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 0; vertexArray[idx++] = 1;

            // Bottom face (54 values)
            vertexArray[idx++] = bbr_x; vertexArray[idx++] = bbr_y; vertexArray[idx++] = bbr_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 0; vertexArray[idx++] = 0;

            vertexArray[idx++] = bbl_x; vertexArray[idx++] = bbl_y; vertexArray[idx++] = bbl_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 1; vertexArray[idx++] = 0;

            vertexArray[idx++] = fbl_x; vertexArray[idx++] = fbl_y; vertexArray[idx++] = fbl_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 0; vertexArray[idx++] = 1;

            vertexArray[idx++] = bbl_x; vertexArray[idx++] = bbl_y; vertexArray[idx++] = bbl_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 1; vertexArray[idx++] = 0;

            vertexArray[idx++] = fbr_x; vertexArray[idx++] = fbr_y; vertexArray[idx++] = fbr_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 1; vertexArray[idx++] = 1;

            vertexArray[idx++] = fbl_x; vertexArray[idx++] = fbl_y; vertexArray[idx++] = fbl_z;
            vertexArray[idx++] = r; vertexArray[idx++] = g; vertexArray[idx++] = b; vertexArray[idx++] = 255;
            vertexArray[idx++] = 0; vertexArray[idx++] = 1;
        };

        // Generate grid lines
        for (let i = -Math.floor(halfLength); i < Math.floor(halfLength); i++) {
            const lineWidth = (i === 0) ? 0.3 : 0.05;
            const halfWidth = lineWidth / 2.0;

            // Color selection
            const isColored = (i % 5 === 0);
            const colorX_r = isColored ? 255 : 64;
            const colorX_g = isColored ? 0 : 64;
            const colorX_b = isColored ? 0 : 64;
            const colorY_r = isColored ? 0 : 64;
            const colorY_g = isColored ? 255 : 64;
            const colorY_b = isColored ? 0 : 64;

            // X-axis line
            addBox(-halfLength, -halfWidth + i, -halfWidth,
                   halfLength, halfWidth + i, halfWidth,
                   colorX_r, colorX_g, colorX_b);

            // Y-axis line
            addBox(-halfWidth + i, -halfLength, -halfWidth,
                   halfWidth + i, halfLength, halfWidth,
                   colorY_r, colorY_g, colorY_b);
        }

        // Submit all vertices in single call
        renderer.addVertexBatch(vertexArray);
        const totalVertices = numLines * 2 * verticesPerBox;
        console.log(`RendererSystem: Successfully added ${totalVertices} vertices for grid (${numLines} lines)`);
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

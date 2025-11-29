# DLL Auto-Deployment Enhancement Plan

## Executive Summary

**Objective**: Extend the existing V8 DLL auto-deployment mechanism to include FMOD and OpenSSL runtime dependencies, ensuring consistent DLL availability across all game projects that depend on the Engine.

**Current State**:
- ✅ V8 DLLs: Auto-deployed via PostBuildEvent in Game.vcxproj
- ⚠️ FMOD DLLs: Manually placed in Run/ folder (no auto-deployment)
- ❌ OpenSSL DLLs: Not present in Run/ folder (will cause runtime errors when using KADI WebSocket)

**Proposed Solution**: Implement MSBuild PostBuildEvent commands to auto-copy FMOD and OpenSSL DLLs to the Run/ directory, similar to V8's deployment mechanism.

---

## Problem Analysis

### Current Architecture

```
┌─────────────────────────────────────────────────────────────┐
│ Engine.vcxproj (Static Library)                             │
├─────────────────────────────────────────────────────────────┤
│ • V8 NuGet Package Integration                               │
│   - v8.redist-v143-x64.props: Auto-defines ReferenceCopyLocalPaths │
│   - Copies *.dll to Temporary/ during build                 │
│                                                              │
│ • FMOD Static Linking                                        │
│   - fmod_vc.lib linked statically                           │
│   - fmod.dll required at runtime (NOT auto-copied)          │
│                                                              │
│ • OpenSSL Static Linking                                     │
│   - libssl.lib, libcrypto.lib linked statically             │
│   - libssl-3-x64.dll, libcrypto-3-x64.dll required at runtime │
│   - legacy.dll for legacy algorithms (NOT auto-copied)      │
└─────────────────────────────────────────────────────────────┘
                          │
                          ▼
┌─────────────────────────────────────────────────────────────┐
│ Game.vcxproj (Executable Application)                       │
├─────────────────────────────────────────────────────────────┤
│ PostBuildEvent (Current):                                    │
│ ┌─────────────────────────────────────────────────────────┐ │
│ │ xcopy /Y /F "$(TargetPath)" "$(SolutionDir)Run/"        │ │
│ │ xcopy /Y /F "$(V8RedistLibPath)" "$(SolutionDir)Run/"   │ │
│ └─────────────────────────────────────────────────────────┘ │
│                                                              │
│ Result: ✅ V8 DLLs deployed automatically                    │
│         ⚠️ FMOD DLLs manually placed (outdated risk)        │
│         ❌ OpenSSL DLLs missing (runtime failures)          │
└─────────────────────────────────────────────────────────────┘
```

### Issues Identified

1. **FMOD DLL Management**
   - **Current**: Manually copied to Run/ (last updated Sep 20)
   - **Risk**: Version mismatch if Engine updates FMOD but Run/ DLLs not updated
   - **Impact**: Potential crashes or missing audio features

2. **OpenSSL DLL Absence**
   - **Current**: OpenSSL DLLs NOT in Run/ folder
   - **Risk**: KADI WebSocket connections will fail at runtime
   - **Impact**: Assignment 7 multi-agent collaboration system non-functional
   - **Error**: "The code execution cannot proceed because libssl-3-x64.dll was not found"

3. **V8 DLL Deployment (Working Correctly)**
   - **Current**: Auto-deployed via PostBuildEvent using $(V8RedistLibPath)
   - **Mechanism**: V8 NuGet package defines MSBuild properties for DLL paths
   - **Result**: Always up-to-date, no manual intervention needed

---

## Technical Investigation

### V8 Auto-Deployment Mechanism (Reference Implementation)

**File**: `ProtogameJS3D\Code\Game\Game.vcxproj` (Lines 195-202)

```xml
<PostBuildEvent Condition="'$(EnableScriptModule)'=='true'">
  <Command>xcopy /Y /F /I "$(TargetPath)" "$(SolutionDir)Run/" &amp; xcopy /Y /F "$(V8RedistLibPath)" "$(SolutionDir)Run/"</Command>
  <Message>Deploying $(TargetFileName) and V8 Debug runtime to game directory...</Message>
</PostBuildEvent>
```

**How V8RedistLibPath is Defined**:

**File**: `Engine\Code\ThirdParty\packages\v8.redist-v143-x64.13.0.245.25\build\native\v8.redist-v143-x64.props`

```xml
<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Condition="'$(PlatformToolset)' == 'v143' And '$(Platform)' == 'x64'">
    <ReferenceCopyLocalPaths Include="$(MSBuildThisFileDirectory)..\..\lib\$(Configuration)\*.dll" />
    <ReferenceCopyLocalPaths Include="$(MSBuildThisFileDirectory)..\..\lib\$(Configuration)\icudtl*.dat" />
    <ReferenceCopyLocalPaths Include="$(MSBuildThisFileDirectory)..\..\lib\$(Configuration)\*_blob.bin" />
  </ItemGroup>
</Project>
```

**Key Insights**:
- V8 NuGet package provides `.props` file that MSBuild imports
- `ReferenceCopyLocalPaths` tells MSBuild to copy DLLs to output directory
- Game project PostBuildEvent copies from Temporary/ to Run/
- Configuration-aware: Debug/Release DLLs selected automatically

### FMOD DLL Locations

**Engine Repository**:
```
Engine\Code\ThirdParty\fmod\
├── lib\
│   ├── x64\
│   │   ├── fmod_vc.lib        (Static import library)
│   │   └── fmod64.dll         (64-bit runtime - Release)
│   └── x86\
│       ├── fmod_vc.lib
│       └── fmod.dll           (32-bit runtime)
└── include\
    ├── fmod.h
    ├── fmod.hpp
    └── fmod_errors.h
```

**Current Status**:
- Both `fmod.dll` (x86) and `fmod64.dll` (x64) are in Run/ folder
- Manually placed on Sep 20, 2025
- No PostBuildEvent to auto-update

**Recommended Deployment**:
- x64 builds → Copy `fmod64.dll`
- x86 builds → Copy `fmod.dll`

### OpenSSL DLL Locations

**Engine Repository** (Recently Added via Git LFS):
```
Engine\Code\ThirdParty\openssl\
├── bin\
│   └── x64\
│       ├── Debug\
│       │   ├── libssl-3-x64.dll       (6.5 MB)
│       │   ├── libcrypto-3-x64.dll    (5.4 MB)
│       │   └── legacy.dll             (220 KB)
│       └── Release\
│           ├── libssl-3-x64.dll       (6.2 MB)
│           ├── libcrypto-3-x64.dll    (5.2 MB)
│           └── legacy.dll             (216 KB)
├── lib\
│   └── x64\
│       ├── Debug\
│       │   ├── libssl.lib
│       │   └── libcrypto.lib
│       └── Release\
│           ├── libssl.lib
│           └── libcrypto.lib
└── include\
    └── openssl\
        └── *.h (headers)
```

**Current Status**:
- ❌ NOT present in Run/ folder
- Required for KADI WebSocket (wss:// secure connections)
- Will cause runtime DLL load errors when Network module initializes

---

## Proposed Solution

### Implementation Strategy

**Approach 1: PostBuildEvent Extension (Recommended)**

Extend Game.vcxproj's PostBuildEvent to copy FMOD and OpenSSL DLLs alongside V8 DLLs.

**Advantages**:
- ✅ Simple implementation
- ✅ Consistent with existing V8 pattern
- ✅ No Engine.vcxproj changes needed
- ✅ Configuration-aware (Debug/Release)
- ✅ Platform-aware (x86/x64)

**Disadvantages**:
- ⚠️ Must be replicated in every Game project that uses Engine
- ⚠️ Manual maintenance if new dependencies added

**Approach 2: Custom MSBuild .props File**

Create `Engine.DllDeployment.props` file that Engine.vcxproj exports and Game projects import.

**Advantages**:
- ✅ Centralized DLL deployment logic
- ✅ Easier to maintain across multiple game projects
- ✅ Professional MSBuild integration pattern

**Disadvantages**:
- ⚠️ More complex initial setup
- ⚠️ Requires understanding MSBuild property system

**Decision**: Use **Approach 1** for immediate implementation, then migrate to **Approach 2** if additional game projects are created.

---

## Implementation Plan

### Phase 1: FMOD DLL Auto-Deployment

**File**: `ProtogameJS3D\Code\Game\Game.vcxproj`

**Current PostBuildEvent (Debug|x64)**:
```xml
<PostBuildEvent Condition="'$(EnableScriptModule)'=='true'">
  <Command>xcopy /Y /F /I "$(TargetPath)" "$(SolutionDir)Run/" &amp; xcopy /Y /F "$(V8RedistLibPath)" "$(SolutionDir)Run/"</Command>
  <Message>Deploying $(TargetFileName) and V8 Debug runtime to game directory...</Message>
</PostBuildEvent>
```

**Enhanced PostBuildEvent (Debug|x64)**:
```xml
<PostBuildEvent Condition="'$(EnableScriptModule)'=='true'">
  <Command>
    xcopy /Y /F /I "$(TargetPath)" "$(SolutionDir)Run/" &amp;
    xcopy /Y /F "$(V8RedistLibPath)" "$(SolutionDir)Run/" &amp;
    xcopy /Y /F "$(SolutionDir)../Engine/Code/ThirdParty/fmod/lib/x64/fmod64.dll" "$(SolutionDir)Run/"
  </Command>
  <Message>Deploying $(TargetFileName), V8 Debug runtime, and FMOD runtime to game directory...</Message>
</PostBuildEvent>
```

**Changes for Each Configuration**:

| Configuration | Platform | FMOD DLL to Copy |
|---------------|----------|------------------|
| Debug         | Win32    | `fmod/lib/x86/fmod.dll` |
| Debug         | x64      | `fmod/lib/x64/fmod64.dll` |
| Release       | Win32    | `fmod/lib/x86/fmod.dll` |
| Release       | x64      | `fmod/lib/x64/fmod64.dll` |
| DebugInline   | x64      | `fmod/lib/x64/fmod64.dll` |
| FastBreak     | x64      | `fmod/lib/x64/fmod64.dll` |

### Phase 2: OpenSSL DLL Auto-Deployment

**Enhanced PostBuildEvent (Debug|x64)**:
```xml
<PostBuildEvent Condition="'$(EnableScriptModule)'=='true'">
  <Command>
    xcopy /Y /F /I "$(TargetPath)" "$(SolutionDir)Run/" &amp;
    xcopy /Y /F "$(V8RedistLibPath)" "$(SolutionDir)Run/" &amp;
    xcopy /Y /F "$(SolutionDir)../Engine/Code/ThirdParty/fmod/lib/x64/fmod64.dll" "$(SolutionDir)Run/" &amp;
    xcopy /Y /F "$(SolutionDir)../Engine/Code/ThirdParty/openssl/bin/x64/Debug/*.dll" "$(SolutionDir)Run/"
  </Command>
  <Message>Deploying $(TargetFileName), V8 Debug runtime, FMOD runtime, and OpenSSL Debug runtime to game directory...</Message>
</PostBuildEvent>
```

**OpenSSL DLLs Per Configuration**:

| Configuration | Platform | OpenSSL DLL Source |
|---------------|----------|-------------------|
| Debug         | x64      | `openssl/bin/x64/Debug/*.dll` (3 files) |
| Release       | x64      | `openssl/bin/x64/Release/*.dll` (3 files) |
| DebugInline   | x64      | `openssl/bin/x64/Release/*.dll` (optimized) |
| FastBreak     | x64      | `openssl/bin/x64/Release/*.dll` (optimized) |

### Phase 3: Conditional Deployment Logic

Add conditional flags for optional subsystems:

```xml
<!-- Property Group Definitions -->
<PropertyGroup>
  <EnableFMODAudio>true</EnableFMODAudio>
  <EnableSecureNetwork>true</EnableSecureNetwork>
</PropertyGroup>

<!-- Conditional PostBuildEvent -->
<PostBuildEvent>
  <Command>
    xcopy /Y /F /I "$(TargetPath)" "$(SolutionDir)Run/"

    <!-- V8 Deployment (Existing) -->
    if '$(EnableScriptModule)'=='true' (
      xcopy /Y /F "$(V8RedistLibPath)" "$(SolutionDir)Run/"
    )

    <!-- FMOD Deployment -->
    if '$(EnableFMODAudio)'=='true' (
      xcopy /Y /F "$(SolutionDir)../Engine/Code/ThirdParty/fmod/lib/$(Platform)/fmod*.dll" "$(SolutionDir)Run/"
    )

    <!-- OpenSSL Deployment -->
    if '$(EnableSecureNetwork)'=='true' (
      xcopy /Y /F "$(SolutionDir)../Engine/Code/ThirdParty/openssl/bin/$(Platform)/$(Configuration)/*.dll" "$(SolutionDir)Run/"
    )
  </Command>
</PostBuildEvent>
```

---

## Acceptance Criteria

### Phase 1 - FMOD Auto-Deployment

✅ **AC1.1**: FMOD DLL auto-copies to Run/ on every build
✅ **AC1.2**: Correct DLL selected based on platform (x86 vs x64)
✅ **AC1.3**: Run/ folder contains up-to-date FMOD DLL matching Engine version
✅ **AC1.4**: Audio subsystem loads successfully without manual DLL placement
✅ **AC1.5**: No build errors or warnings related to FMOD DLL copying

### Phase 2 - OpenSSL Auto-Deployment

✅ **AC2.1**: OpenSSL DLLs (libssl, libcrypto, legacy) auto-copy to Run/
✅ **AC2.2**: Correct DLL configuration selected (Debug vs Release)
✅ **AC2.3**: KADI WebSocket subsystem initializes successfully
✅ **AC2.4**: Secure wss:// connections work without "DLL not found" errors
✅ **AC2.5**: Git LFS tracks OpenSSL DLLs correctly (no repository bloat)

### Phase 3 - Conditional Deployment

✅ **AC3.1**: DLL deployment respects EnableFMODAudio property
✅ **AC3.2**: DLL deployment respects EnableSecureNetwork property
✅ **AC3.3**: Minimal game projects can disable optional DLLs
✅ **AC3.4**: Build configuration clearly shows which DLLs are deployed

---

## Risk Assessment

### High Priority Risks

**Risk R1: Path Resolution Failures**
- **Description**: Relative paths to Engine DLLs may fail if solution structure changes
- **Mitigation**: Use $(SolutionDir) and test all configurations
- **Likelihood**: Medium
- **Impact**: High (build breaks)

**Risk R2: xcopy Command Failures**
- **Description**: xcopy may fail if destination folder doesn't exist or files are locked
- **Mitigation**: Use `/I` flag (create directory), `/Y` (overwrite), `/F` (show full paths)
- **Likelihood**: Low
- **Impact**: Medium (manual copy needed)

**Risk R3: Git LFS Pull Failures**
- **Description**: Developers may not have OpenSSL DLLs if Git LFS not configured
- **Mitigation**: Document Git LFS setup in README, add validation script
- **Likelihood**: Medium
- **Impact**: High (missing DLLs)

### Medium Priority Risks

**Risk R4: Configuration Mismatches**
- **Description**: Debug build might copy Release DLLs or vice versa
- **Mitigation**: Use $(Configuration) variable, test all build configurations
- **Likelihood**: Low
- **Impact**: Medium (hard-to-debug crashes)

**Risk R5: Platform Mismatches**
- **Description**: x64 build might copy x86 DLLs
- **Mitigation**: Use $(Platform) variable correctly
- **Likelihood**: Low
- **Impact**: High (DLL architecture mismatch error)

---

## Testing Strategy

### Unit Testing (Per Configuration)

Test each configuration independently:

| Test Case | Configuration | Platform | Expected DLLs |
|-----------|---------------|----------|---------------|
| TC1       | Debug         | x64      | V8 (Debug), fmod64.dll, OpenSSL (Debug) |
| TC2       | Release       | x64      | V8 (Release), fmod64.dll, OpenSSL (Release) |
| TC3       | DebugInline   | x64      | V8 (Debug), fmod64.dll, OpenSSL (Release) |
| TC4       | FastBreak     | x64      | V8 (Release), fmod64.dll, OpenSSL (Release) |

### Integration Testing

**IT1**: Clean Build Test
1. Delete all DLLs from Run/ folder
2. Build ProtogameJS3D in Debug|x64
3. Verify all DLLs present in Run/
4. Run application, confirm no "DLL not found" errors

**IT2**: Incremental Build Test
1. Build once (DLLs copied)
2. Touch a source file
3. Build again
4. Verify DLLs are re-copied (timestamps updated)

**IT3**: Audio Subsystem Test
1. Build and run application
2. Trigger audio playback (FMOD API call)
3. Verify audio plays without errors
4. Check logs for FMOD initialization success

**IT4**: KADI WebSocket Test
1. Build and run application
2. Initialize KADI WebSocket subsystem
3. Attempt wss:// connection to broker
4. Verify TLS handshake succeeds
5. Confirm OpenSSL DLLs loaded successfully

### Regression Testing

**RT1**: V8 DLL Deployment (Existing Functionality)
- Ensure V8 DLLs still copy correctly
- Verify JavaScript execution works
- Check hot-reload functionality

**RT2**: Build Performance
- Measure build time before/after changes
- Ensure xcopy commands don't significantly slow builds
- Target: < 2 second additional build time

---

## Rollback Plan

If DLL auto-deployment causes issues:

### Step 1: Immediate Rollback
```bash
# Revert Game.vcxproj changes
git checkout HEAD -- Code/Game/Game.vcxproj

# Manually copy DLLs to Run/
xcopy /Y "Engine/Code/ThirdParty/fmod/lib/x64/fmod64.dll" "Run/"
xcopy /Y "Engine/Code/ThirdParty/openssl/bin/x64/Debug/*.dll" "Run/"
```

### Step 2: Verify Rollback
1. Build ProtogameJS3D
2. Check Run/ folder for DLLs
3. Run application, test audio and network

### Step 3: Document Issues
- Create GitHub issue with error logs
- Include MSBuild output
- Note which configuration failed

---

## Future Enhancements

### FE1: Centralized DLL Deployment Props File

Create `Engine.DllDeployment.props`:
```xml
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup>
    <EngineDllDeployment>true</EngineDllDeployment>
  </PropertyGroup>

  <Target Name="DeployEngineDlls" AfterTargets="PostBuildEvent" Condition="'$(EngineDllDeployment)'=='true'">
    <ItemGroup>
      <FMODDlls Include="$(EngineRoot)/ThirdParty/fmod/lib/$(Platform)/fmod*.dll" />
      <OpenSSLDlls Include="$(EngineRoot)/ThirdParty/openssl/bin/$(Platform)/$(Configuration)/*.dll" />
    </ItemGroup>

    <Copy SourceFiles="@(FMODDlls)" DestinationFolder="$(TargetDir)" SkipUnchangedFiles="true" />
    <Copy SourceFiles="@(OpenSSLDlls)" DestinationFolder="$(TargetDir)" SkipUnchangedFiles="true" />
  </Target>
</Project>
```

**Benefits**:
- Single source of truth for DLL deployment
- Reusable across multiple game projects
- Professional MSBuild integration

### FE2: DLL Version Validation

Add build-time validation:
```xml
<Target Name="ValidateDllVersions" BeforeTargets="Build">
  <Warning Text="FMOD DLL version mismatch detected" Condition="..." />
  <Warning Text="OpenSSL DLL version mismatch detected" Condition="..." />
</Target>
```

### FE3: Git LFS Setup Automation

Add setup script:
```bash
# setup-git-lfs.bat
@echo off
echo Installing Git LFS hooks...
git lfs install

echo Pulling LFS files...
git lfs pull

echo Verifying DLL presence...
if not exist "Engine\Code\ThirdParty\openssl\bin\x64\Debug\libssl-3-x64.dll" (
    echo ERROR: OpenSSL DLLs not found. Run 'git lfs pull' manually.
    exit /b 1
)

echo Git LFS setup complete!
```

---

## Implementation Checklist

### Pre-Implementation
- [ ] Review existing PostBuildEvent structure
- [ ] Verify Engine DLL paths are correct
- [ ] Test Git LFS for OpenSSL DLLs
- [ ] Backup current Game.vcxproj

### Phase 1: FMOD Auto-Deployment
- [ ] Update Debug|x64 PostBuildEvent
- [ ] Update Release|x64 PostBuildEvent
- [ ] Update DebugInline|x64 PostBuildEvent
- [ ] Update FastBreak|x64 PostBuildEvent
- [ ] Test each configuration builds successfully
- [ ] Verify fmod64.dll copied to Run/
- [ ] Test audio playback in application

### Phase 2: OpenSSL Auto-Deployment
- [ ] Add OpenSSL xcopy commands to PostBuildEvents
- [ ] Verify Debug vs Release DLL selection
- [ ] Test Git LFS pull for OpenSSL DLLs
- [ ] Verify all 3 OpenSSL DLLs copied to Run/
- [ ] Test KADI WebSocket initialization
- [ ] Test wss:// secure connection

### Phase 3: Documentation
- [ ] Update README with DLL auto-deployment info
- [ ] Document Git LFS setup for OpenSSL
- [ ] Add troubleshooting section for DLL errors
- [ ] Create CLAUDE.md updates for build system

### Post-Implementation
- [ ] Run full integration test suite
- [ ] Verify no performance regression
- [ ] Create git commit with changes
- [ ] Update project documentation

---

## Conclusion

This plan provides a comprehensive approach to extending DLL auto-deployment from V8-only to include FMOD and OpenSSL. The phased implementation minimizes risk while ensuring all runtime dependencies are consistently available.

**Key Benefits**:
- ✅ Eliminates manual DLL management
- ✅ Prevents version mismatches
- ✅ Ensures OpenSSL availability for KADI
- ✅ Consistent with existing V8 pattern
- ✅ Maintains build system reliability

**Next Steps**: Review and approve plan, then proceed with Phase 1 implementation.

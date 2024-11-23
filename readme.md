###TODO:
- Calculate (and set) bounds outside of draw 
- Fix update on dark mode when switching
- Image invert function
    - Could be sent and done on the server
- Spotify track max length
- Standardize the button width / height



```mermaid
graph TD;
    subgraph Application Flow
        A[Application Start] --> B[Initialize Display]
        B --> C[Create ElementManager]
        C --> D[Main Loop]
        
        D -->|Every 20ms| E[Check Auto Update]
        D -->|Continuous| F[ElementManager Loop]
        D -->|On Refresh| G[Refresh Display]
    end

    subgraph Element Manager Loop
        F --> H{Update Queue Empty?}
        H -->|No| I[Process Next Update]
        I --> J{Needs Clear?}
        J -->|Yes| K[Clear Element Area]
        J -->|No| L{Needs Draw?}
        K --> L
        L -->|Yes| M[Draw Element]
        L -->|No| H
        M --> H
        
        H -->|Yes| N{Any Touched Elements?}
        N -->|Yes| O[Process Touch Events]
        O --> P[Draw Touched State]
        P --> Q[Execute Callback]
        Q -->|Create Task| R[Touch Response Task]
    end

    subgraph Element Updates
        G --> S[Get API Response]
        S --> T[Process JSON Elements]
        T --> U{Element Exists?}
        U -->|Yes| V{Element Changed?}
        U -->|No| W[Create New Element]
        V -->|Yes| X[Queue Clear & Update]
        V -->|No| Y[Mark Updated]
        W --> Z[Queue Draw]
    end

    subgraph Refresh Types
        AA[Refresh Types]
        AA --> BB[FAST]
        AA --> CC[PARTIAL]
        AA --> DD[COMPLETE]
        BB -->|1 cycle| EE[Clear & Draw]
        CC -->|2 cycles| EE
        DD -->|4 cycles| EE
    end
```
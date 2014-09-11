#include <functional>
#include <iostream>
#include <vector>
#include <qmatrix4x4.h>

#include "BSPTreeMesh.h"
#include "Mesh.h"



/**
 * @brief BSPTreeMesh::BSPTreeMesh
 */
BSPTreeMesh::BSPTreeMesh()
{
    mBSPTreeRoot = NULL;
}


/**
 * @brief BSPTreeMesh::save salva il BSPTree
 * @param filename path di salvataggio
 * @return true se l'operazione e' andata a buon fine
 */
bool BSPTreeMesh::save (const std::string &filename)
{
    // Il file designato e' filename + ".tree"
    return false;
}


/**
 * @brief BSPTreeMesh::load carica il BSPTree
 * @param filename path di caricamento
 * @return true se l'operazione e' andata a buon fine
 */
bool BSPTreeMesh::load (const std::string &filename)
{
    // Il file designato e' filename + ".tree"
    return false;
}


/**
 * @brief BSPTreeMesh::draw
 */
void BSPTreeMesh::draw(/* QUI CI SARA IL PUNTO DI VISTO (SE MI LASCIATE LA LIBERTA') */)
{
#ifdef DEBUG_TRIANGULATION
    Mesh::draw ();
    return;
#endif

    Vertex pov;
    pov.p.init(0.0,0.0,0.0);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glVertexPointer(3, GL_FLOAT, sizeof (Vertex), (GLvoid*)(&V()[0].p));
    glNormalPointer(GL_FLOAT, sizeof (Vertex), (GLvoid*)(((float*)&V()[0].p) + 3));

    _draw (mBSPTreeRoot, pov);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
}


/**
 * @brief BSPTreeMesh::_draw
 * @param root
 * @param pov
 */
void BSPTreeMesh::_draw (BSPNode *root, Vertex pov)
{
    if (root == NULL) return;

    if (BSPLeafNode* leaf = dynamic_cast<BSPLeafNode*>(root))
    {
        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (GLvoid*)(&leaf->NodeTriangle));
    }
    else if (BSPInternalNode* internal = dynamic_cast<BSPInternalNode*>(root))
    {
        double det = determinant (internal->NodeTriangles[0], pov);
        Position pos = determinantToPosition(det);

        switch (pos)
        {
        case POS_RIGHT:
            _draw(internal->Left, pov);
            glDrawElements(GL_TRIANGLES, 3 * internal->NodeTriangles.size(),
                           GL_UNSIGNED_INT, (GLvoid*)(&internal->NodeTriangles[0]));
            _draw(internal->Right, pov);
            break;

        case POS_LEFT:
            _draw(internal->Right, pov);
            glDrawElements(GL_TRIANGLES, 3 * internal->NodeTriangles.size(),
                           GL_UNSIGNED_INT, (GLvoid*)(&internal->NodeTriangles[0]));
            _draw(internal->Left, pov);
            break;

        default:
            _draw(internal->Right, pov);
            _draw(internal->Left, pov);
            break;
        }
    }
}




/**
 * @brief BSPTreeMesh::determinantToPosition
 * @param d
 * @return
 */
BSPTreeMesh::Position BSPTreeMesh::determinantToPosition (double d)
{
    if (d < 0.0)
        return POS_LEFT;
    else if (d > 0.0)
        return POS_RIGHT;
    else
        return POS_CENTER;
}



/**
 * @brief BSPTreeMesh::determinant Calcola il determinante di un vertice rispetto ad un piano
 * @param t Piano
 * @param v Vertice da verificare
 * @return determinante
 */
double BSPTreeMesh::determinant (Triangle t, Vertex v)
{
    QMatrix4x4 coplanare;
    double det;

    Vertex v1 = V()[t[0]];
    Vertex v2 = V()[t[1]];
    Vertex v3 = V()[t[2]];
    coplanare.setRow(0, QVector4D (v1.p[0], v1.p[1], v1.p[2], 1));
    coplanare.setRow(1, QVector4D (v2.p[0], v2.p[1], v2.p[2], 1));
    coplanare.setRow(2, QVector4D (v3.p[0], v3.p[1], v3.p[2], 1));
    coplanare.setRow(3, QVector4D (v.p[0], v.p[1], v.p[2], 1));

    det = coplanare.determinant();

    /* Se il valore del determinante e' vicino allo zero, approssimo a zero */
    if (det <= DETERMINANT_ZERO_APPROX && det >= -DETERMINANT_ZERO_APPROX)
        return 0.0;
    else
        return det;
}




/**
 * @brief BSPTreeMesh::createBSPTree
 */
void BSPTreeMesh::createBSPTree ()
{
    std::cout << "BSPTreeMesh::createBSPTree()" << std::endl << std::flush;

    mNodesNumber = 0;

    if (mBSPTreeRoot != NULL)
        delete mBSPTreeRoot;

    /* Randomizzo l'input (I triangoli) */
    std::random_shuffle(T().begin(), T().end());

    /* Crea il bsp tree */
    mBSPTreeRoot = _createBSPTree (T());

    std::cout << "BSPTreeMesh::createBSPTree() ends with " << mNodesNumber
              << " nodes" << std::endl << std::flush;
}


/**
 * @brief BSPTreeMesh::normalOfTriangle Calcola la normale di un triangolo
 * @param t Triangolo di cui calcolare la normale
 * @return Vettore della normale
 */
Vec3Df BSPTreeMesh::normalOfTriangle (Triangle t)
{
    Vec3Df dir = Vec3Df::cross_product(V()[t[1]].p - V()[t[0]].p, V()[t[2]].p - V()[t[0]].p);
    dir.normalize();
    return dir;
}


/**
 * @brief BSPTreeMesh::planeSegmentIntersection Calcola l'intersezione tra un piano
 *              (definito da 3 punti, ed un segmento definito da due vertici)
 * @param plane
 * @param a
 * @param b
 * @return Il punto di intersezione o NULL
 * @todo Verificare se effettivamente funziona come stabilito
 */
Vec3Df* BSPTreeMesh::planeSegmentIntersection (Triangle plane, Vertex a, Vertex b)
{
    Vec3Df ray = a.p - b.p;
    Vec3Df rayOrigin = a.p;
    Vec3Df normal = normalOfTriangle (plane);
    Vec3Df coord = V()[plane[0]].p;

    float d = Vec3Df::dot_product(normal, coord);

    if (Vec3Df::dot_product(normal, ray) == 0.0) //TODO: Or near 0?
    {
        //std::cout << "No intersection, 0\n";
        return NULL;
    }

    // Compute the t value for the directed line ray intersecting the plane
    float t = (d - Vec3Df::dot_product(normal, rayOrigin)) / Vec3Df::dot_product(normal, ray);

    // scale the ray by t
    Vec3Df newRay = ray * t;

    // calc contact point
    Vec3Df *contact = new Vec3Df (rayOrigin + newRay);

    if (t >= 0.0f && t <= 1.0f)
    {
        // Se il punto di intersezione e' un vertice del segmento, lo scarto
        if (*contact == a.p || *contact == b.p)
            return NULL;

        std::cout << contact[0] << " " << contact[1] << " " << contact[2] << std::endl << std::flush;
        return contact;
    }

    //std::cout << "No intersection\n";
    return NULL;
}



/**
 * @brief BSPTreeMesh::_createBSPTree Funzione ricorsiva di creazione del bsptree
 * @param s Triangoli della partizione
 * @return
 */
BSPNode* BSPTreeMesh::_createBSPTree (std::vector<Triangle> s)
{
    if (s.size() <= 1)
    {
        /* Nessun triangolo nel vettore */
        if (s.size() == 0)
            return NULL;

        mNodesNumber++;

        /* Un triangolo nel vettore, creo una foglia */
        BSPLeafNode* leaf = new BSPLeafNode ();
        leaf->NodeTriangle = s.at(0);

        return leaf;
    }
    /* Piu' triangoli nel vettore, suddivo gli insiemi e richiamo la funzione per i figli */
    else
    {
        mNodesNumber++;

        std::vector<Triangle> leftTriangles;
        std::vector<Triangle> rightTriangles;
        std::vector<Triangle> centerTriangles;

        /* Utilizzo il primo triangolo della lista come piano di taglio */
        Triangle subdivisionPlane = s.at(0);

        /* Creo il nodo interno */
        BSPInternalNode* internal = new BSPInternalNode ();

        /* Aggiungo alla lista di quelli centrali, il triangolo utilizzato
         * come suddivisione */
        centerTriangles.push_back(s.at(0));

        /* Controllo in che posizioni stanno i restanti triangoli della lista ed agisco di conseguenza */
        for (unsigned i=1; i<s.size(); i++)
        {
            /* Calcola la posizione di un triangolo rispetto ad
             * un piano di taglio */
            Position pos;

            Position p0 = determinantToPosition(determinant (subdivisionPlane, V()[s[i][0]]));
            Position p1 = determinantToPosition(determinant (subdivisionPlane, V()[s[i][1]]));
            Position p2 = determinantToPosition(determinant (subdivisionPlane, V()[s[i][2]]));

            /* Se le posizioni dei punti del triangolo t rispetto al piano sono uguali,
             * rilascio la posizione comune. Trascuro i casi in cui ho un vertice al centro
             * e gli altri tutti da una parte */
            if (p0 == p1 && p1 == p2 || p0 == POS_CENTER && (p1 == p2)
                    || p1 == POS_CENTER && (p0 == p2) || p2 == POS_CENTER && (p0 == p1))
            {
                if (p0 != POS_CENTER)
                    pos = p0;
                else
                    pos = p1;
            }
            /* Se non sono tutti uguali, ho un intersezione col piano di taglio */
            else
                pos = POS_INTERSECT;


            switch (pos)
            {
            case POS_LEFT:
                leftTriangles.push_back(s.at(i));
                break;

            case POS_CENTER:
                centerTriangles.push_back(s.at(i));
                break;

            case POS_RIGHT:
                rightTriangles.push_back(s.at(i));
                break;

            /* Il triangolo corrente interseca il piano di taglio; trovo i punti di intersezione,
             * triangolo i punti ottenuti, ed aggiungo i nuovi vertici ed i nuovi triangoli alle
             * liste apposite */
            default:
                Vec3Df *intersect1; // Intersection between s[i][0] and s[i][1]
                Vec3Df *intersect2; // Intersection between s[i][0] and s[i][2]
                Vec3Df *intersect3; // Intersection between s[i][1] and s[i][2]

                intersect1 = planeSegmentIntersection (subdivisionPlane, V()[s[i][0]], V()[s[i][1]]);
                intersect2 = planeSegmentIntersection (subdivisionPlane, V()[s[i][0]], V()[s[i][2]]);
                intersect3 = planeSegmentIntersection (subdivisionPlane, V()[s[i][1]], V()[s[i][2]]);

                /* Intersezione su un solo lato; aggiungo tale intersezione sulla lista dei vertici
                 * elimino il triangolo corrente e ne creo due nuovi. Posso avere l'intersezione
                 * con un solo lato dato che la planeSegmentIntersection scarta le intersezioni che
                 * giaciono su un vertice del segmento */
                if (intersect1 || intersect2 || intersect3)
                {
                    // TODO devo sovrascrivere i vecchi triangoli, ma non e' strettamente necessario
                    // visto che non li aggiungo al bsptree
                    if (intersect1)
                    {
                        // L'altro punto di intersezione e' V()[s[i][2]]
                        // I nuovi triangoli sono (0 - int1 - 2) e (int1 - 1 - 2)

                        Vertex v;
                        v.p = *intersect1;
                        v.n = *intersect1;
                        v.n.normalize();
                        V().push_back (v);


                        Triangle t1;
                        Triangle t2;

                        t1.init (s[i][0], V().size()-1, s[i][2]);
                        t2.init (V().size()-1, s[i][1], s[i][2]);


                        /* Aggiungo alla lista apposita */
                        if (p0 == POS_LEFT)
                            leftTriangles.push_back (t1);
                        else
                            rightTriangles.push_back (t1);

                        if (p1 == POS_LEFT)
                            leftTriangles.push_back (t2);
                        else
                            rightTriangles.push_back (t2);


                        /* Li aggiungo entrambi alla lista dei triangoli */
                        T().push_back (t1);
                        T().push_back (t2);

                        std::cout << "I1\n" << std::flush;
                    }
                    else if (intersect2)
                    {
                        // L'altro punto di intersezione e' V()[s[i][1]]
                        // I nuovi triangoli sono (1 - int1 - 2) e (int1 - 1 - 0)

                        Vertex v;
                        v.p = *intersect2;
                        v.n = *intersect2;
                        v.n.normalize();
                        V().push_back (v);


                        Triangle t1;
                        Triangle t2;

                        t1.init (s[i][1], V().size()-1, s[i][2]);
                        t2.init (V().size()-1, s[i][1], s[i][0]);


                        /* Aggiungo alla lista apposita */
                        if (p2 == POS_LEFT)
                            leftTriangles.push_back (t1);
                        else
                            rightTriangles.push_back (t1);

                        if (p0 == POS_LEFT)
                            leftTriangles.push_back (t2);
                        else
                            rightTriangles.push_back (t2);


                        /* Li aggiungo entrambi alla lista dei triangoli */
                        T().push_back (t1);
                        T().push_back (t2);

                        std::cout << "I2\n" << std::flush;
                    }
                    else if (intersect3)
                    {
                        // L'altro punto di intersezione e' V()[s[i][0]]
                        // I nuovi triangoli sono (0 - int1 - 2) e (int1 - 1 - 0)

                        Vertex v;
                        v.p = *intersect3;
                        v.n = *intersect3;
                        v.n.normalize();
                        V().push_back (v);


                        Triangle t1;
                        Triangle t2;

                        t1.init (s[i][0], V().size()-1, s[i][2]);
                        t2.init (V().size()-1, s[i][1], s[i][0]);


                        /* Aggiungo alla lista apposita */
                        if (p2 == POS_LEFT)
                            leftTriangles.push_back (t1);
                        else
                            rightTriangles.push_back (t1);

                        if (p1 == POS_LEFT)
                            leftTriangles.push_back (t2);
                        else
                            rightTriangles.push_back (t2);


                        /* Li aggiungo entrambi alla lista dei triangoli */
                        T().push_back (t1);
                        T().push_back (t2);

                        std::cout << "I3\n" << std::flush;

                    }
                }
                /* Intersezione su piu' lati; aggiungo le intersezioni alla lista dei vertici, e creo i
                 * 3 triangoli risulanti */
                else if (intersect1 && intersect2 || intersect1 && intersect3 || intersect2 && intersect3)
                {
                    if (intersect1 && intersect2)
                    {
                        Vertex v1;
                        v1.p = *intersect1;
                        v1.n = *intersect1;
                        v1.n.normalize();
                        V().push_back (v1);

                        Vertex v2;
                        v2.p = *intersect2;
                        v2.n = *intersect2;
                        v2.n.normalize();
                        V().push_back (v2);

                        Triangle t1;
                        Triangle t2;
                        Triangle t3;

                    }
                    else if (intersect1 && intersect3)
                    {

                    }
                    else if (intersect2 && intersect3)
                    {

                    }
                }

                if (intersect1) delete intersect1;
                if (intersect2) delete intersect2;
                if (intersect3) delete intersect3;
                break;
            }
        }

        internal->Left = _createBSPTree (leftTriangles);
        internal->Right = _createBSPTree (rightTriangles);
        internal->NodeTriangles = centerTriangles;

        return internal;
    }
}




/**
 * @brief BSPTreeMesh::load_OFF
 * @param filename
 */
void BSPTreeMesh::load_OFF (const std::string &filename)
{
    Mesh::load_OFF (filename);

    /* Prova a caricare il bsptree */
    if (load (filename))
    {

    }
    /* Altrimenti lo genero */
    else
    {
        createBSPTree ();
        save (filename);
    }
}
